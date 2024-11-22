#pragma once
#include <chrono>
#include <functional>
#include <unordered_map>
#include <list>
#include <string>
#include <optional>
#include <mutex>
#include <sstream>
#include <shared_mutex>

template<typename Key, typename Value>
class CCache {

public:

    using EvictionCallback = std::function<void(const Key &, const Value &)>;
    using LoggerCallback = std::function<void(const std::string &)>;

    /**
     * @brief Constructor to initialize the cache with optional parameters.
     * @param max_size Maximum number of items the cache can hold.
     * @param ttl_millis Time-to-live for cache entries in milliseconds.
     * @param logger Optional logger function to log cache operations.
     * @param eviction_callback Optional callback function invoked upon eviction.
     */
    explicit CCache(const int max_size = 100, const long long ttl_millis = 1000 * 60 * 60 * 5,
                    LoggerCallback logger = nullptr, EvictionCallback eviction_callback = nullptr)
        : max_size_(max_size), ttl_millis_(ttl_millis),
          logger_(std::move(logger)), eviction_callback_(std::move(eviction_callback)) {
        if (max_size_ <= 0) throw std::invalid_argument("max_size must be greater than zero");
        if (ttl_millis_ <= 0) throw std::invalid_argument("ttl_millis must be greater than zero");
    }

    /**
     * @brief Checks if the cache contains the given key.
     * @param key The key to search for.
     * @return true if the key exists in the cache, false otherwise.
     */
    bool contains(const Key &key) const {
        std::shared_lock lock(mutex_);
        return cache_map_.contains(key);
    }

    /**
     * @brief Retrieves the value associated with the given key.
     *        Evicts the key if expired.
     * @param key The key to retrieve.
     * @return A std::optional with the value if found, or std::nullopt otherwise.
     */
    std::optional<Value> get(const Key &key) {
        std::unique_lock lock(mutex_);
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            auto &entry = it->second;
            if (is_expired(entry.timestamp)) {
                evict(key);
                return std::nullopt;
            }
            update_access_order(key);
            log("GET: " + key_to_string(key));
            return entry.value;
        }
        return std::nullopt;
    }

    /**
     * @brief Adds or updates a key-value pair in the cache.
     *        Evicts the least recently used entry if max_size is exceeded.
     * @param key The key to add or update.
     * @param value The value to associate with the key.
     */
    void put(const Key &key, const Value &value) {
        std::unique_lock lock(mutex_);
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            update_existing_entry(it, value);
        } else {
            if (cache_map_.size() >= max_size_) evict_lru();
            add_new_entry(key, value);
        }
    }

    /**
     * @brief Evicts the least recently used (LRU) item from the cache.
     */
    void evict_lru() {
        if (access_order_.empty()) return;
        auto lru_key = access_order_.front();
        evict(lru_key);
    }

    /**
     * @brief Evicts the specified key from the cache.
     * @param key The key to evict.
     */
    void evict(const Key &key) {
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            if (eviction_callback_) eviction_callback_(key, it->second.value);
            cache_map_.erase(it);
            access_order_.remove(key);
            log("EVICT: " + key_to_string(key));
        }
    }

    /**
     * @brief Tries to retrieve a value from the cache or computes and caches it if absent.
     * @param key The key to look up or compute.
     * @param compute_func A function to compute the value if not cached.
     * @return The cached or computed value.
     */
    std::optional<Value> with_cache(const Key &key, const std::function<Value()> &compute_func) {
        if (auto cached_value = get(key)) {
            log("HIT: " + key_to_string(key));
            return cached_value;
        }
        log("MISS: " + key_to_string(key));
        Value computed_value = compute_func();
        put(key, computed_value);
        return computed_value;
    }

    /**
     * @brief Clears all entries from the cache.
     */
    void clear() {
        std::unique_lock lock(mutex_);
        cache_map_.clear();
        access_order_.clear();
        log("CLEAR: All cache entries have been removed.");
    }

private:

    struct CacheEntry {
        Value value;
        std::chrono::steady_clock::time_point timestamp;
    };

    int max_size_;
    long long ttl_millis_;
    std::unordered_map<Key, CacheEntry> cache_map_;
    std::list<Key> access_order_;
    LoggerCallback logger_;
    EvictionCallback eviction_callback_;
    mutable std::shared_mutex mutex_;

    /**
     * @brief Checks if a cache entry has expired.
     */
    [[nodiscard]] bool is_expired(const std::chrono::steady_clock::time_point &timestamp) const {
        return std::chrono::steady_clock::now() - timestamp > std::chrono::milliseconds(ttl_millis_);
    }

    /**
     * @brief Updates the access order for LRU tracking.
     */
    void update_access_order(const Key &key) {
        access_order_.remove(key);
        access_order_.push_back(key);
    }

    /**
     * @brief Adds a new entry to the cache.
     */
    void add_new_entry(const Key &key, const Value &value) {
        cache_map_[key] = {value, std::chrono::steady_clock::now()};
        access_order_.push_back(key);
        log("PUT: " + key_to_string(key));
    }

    /**
     * @brief Updates an existing cache entry.
     */
    void update_existing_entry(typename std::unordered_map<Key, CacheEntry>::iterator it, const Value &value) {
        it->second.value = value;
        it->second.timestamp = std::chrono::steady_clock::now();
        update_access_order(it->first);
        log("UPDATE: " + key_to_string(it->first));
    }

    /**
     * @brief Converts a key to a string for logging.
     */
    std::string key_to_string(const Key &key) const {
        if constexpr (std::is_convertible_v<Key, std::string>) {
            return key;
        } else {
            std::stringstream ss;
            ss << key;
            return ss.str();
        }
    }

    /**
     * @brief Logs cache operations if a logger is provided.
     */
    void log(const std::string &message) const {
        if (logger_) logger_(message);
    }

};
