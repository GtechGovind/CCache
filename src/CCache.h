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
     * @brief Constructs a cache with configurable parameters.
     *
     * @param max_size Maximum number of entries the cache can hold. Default is 100.
     * @param ttl_millis Time-to-live for cache entries in milliseconds. Default is 5 hours.
     * @param logger Optional logger function to log cache operations. Default is nullptr.
     * @param eviction_callback Optional callback invoked when an entry is evicted. Default is nullptr.
     *
     * @throws std::invalid_argument if max_size or ttl_millis is less than or equal to 0.
     */
    explicit CCache(const int max_size = 100, const long long ttl_millis = 1000 * 60 * 60 * 5,
                    LoggerCallback logger = nullptr, EvictionCallback eviction_callback = nullptr)
        : max_size_(max_size), ttl_millis_(ttl_millis),
          logger_(std::move(logger)), eviction_callback_(std::move(eviction_callback)) {

        // Validate parameters
        if (max_size_ <= 0) throw std::invalid_argument("max_size must be greater than zero");
        if (ttl_millis_ <= 0) throw std::invalid_argument("ttl_millis must be greater than zero");
    }

    /**
     * @brief Checks if the cache contains a given key.
     *
     * @param key The key to search for in the cache.
     * @return true if the key exists in the cache, false otherwise.
     */
    bool contains(const Key &key) const {
        std::shared_lock lock(mutex_);
        return cache_map_.contains(key);
    }

    /**
     * @brief Retrieves a value from the cache by key, evicting it if expired.
     *
     * If the value has expired, it is evicted and `std::nullopt` is returned.
     *
     * @param key The key to retrieve the associated value.
     * @return A std::optional containing the value if it exists and is not expired, or std::nullopt if the key is absent or expired.
     */
    std::optional<Value> get(const Key &key) {
        std::unique_lock lock(mutex_);

        // Check if the key exists in the cache
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            auto &entry = it->second;

            // Check if the cache entry has expired
            if (is_expired(entry.timestamp)) {
                evict(key);  // Evict expired entry
                return std::nullopt;
            }

            // Update the access order for LRU tracking
            update_access_order(key);

            // Log the GET operation
            log("GET: " + key_to_string(key));

            return entry.value;
        }

        // Return nullopt if the key doesn't exist in the cache
        return std::nullopt;
    }

    /**
     * @brief Inserts or updates a key-value pair in the cache.
     *
     * If the cache reaches its maximum size, the least recently used (LRU) entry is evicted.
     *
     * @param key The key to insert or update.
     * @param value The value to associate with the key.
     * @return A std::optional containing the old value if the key was updated, or std::nullopt if the key was newly inserted.
     */
    std::optional<Value> put(const Key &key, const Value &value) {
        std::unique_lock lock(mutex_);

        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            // If the key exists, update the existing entry and return the old value
            auto old_value = it->second.value;
            update_existing_entry(it, value);
            return old_value;
        } else {
            // If the key does not exist, evict LRU entry if cache is full and add the new entry
            if (cache_map_.size() >= max_size_) evict_lru();
            add_new_entry(key, value);
            return std::nullopt;
        }
    }

    /**
     * @brief Evicts the least recently used (LRU) entry from the cache.
     *
     * This method will remove the least recently used entry based on the access order.
     */
    void evict_lru() {
        if (access_order_.empty()) return;
        auto lru_key = access_order_.front();
        evict(lru_key);
    }

    /**
     * @brief Evicts a specified key from the cache.
     *
     * @param key The key to evict.
     * @return A std::optional containing the evicted value if the key exists, or std::nullopt if the key is not found.
     */
    std::optional<Value> evict(const Key &key) {
        std::unique_lock lock(mutex_);

        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            auto evicted_value = it->second.value;

            // If an eviction callback is set, invoke it
            if (eviction_callback_) eviction_callback_(key, evicted_value);

            // Remove the key from the cache
            cache_map_.erase(it);
            access_order_.remove(key);

            // Log the eviction
            log("EVICT: " + key_to_string(key));

            return evicted_value;
        }

        // Return nullopt if the key is not found
        return std::nullopt;
    }

    /**
     * @brief Retrieves a value from the cache, or computes and caches it if absent.
     *
     * This method attempts to retrieve the cached value, and if it doesn't exist, it computes the value using a provided function
     * and inserts it into the cache. The method logs cache hits and misses.
     *
     * @param key The key to look up or compute.
     * @param compute_func A function to compute the value if it's not already cached.
     * @return The cached or computed value, or std::nullopt if the computed value is absent.
     */
    std::optional<Value> with_cache(const Key& key, const std::function<std::optional<Value>()>& compute_func) {
        // Attempt to retrieve the cached value
        auto cached_value = get(key);
        if (cached_value) {
            log("HIT: " + key_to_string(key));
            return *cached_value;
        }

        // If not found, compute and cache the value
        log("MISS: " + key_to_string(key));
        std::optional<Value> computed_value = compute_func();
        if (computed_value) put(key, *computed_value);
        return computed_value;
    }

    /**
     * @brief Clears all entries from the cache.
     *
     * This method removes all entries from the cache and logs the operation.
     */
    void clear() {
        std::unique_lock lock(mutex_);
        cache_map_.clear();
        access_order_.clear();
        log("CLEAR: All cache entries have been removed.");
    }

private:

    struct CacheEntry {
        Value value;  ///< The cached value.
        std::chrono::steady_clock::time_point timestamp;  ///< The timestamp when the cache entry was created or updated.
    };

    int max_size_;  ///< Maximum number of items the cache can hold.
    long long ttl_millis_;  ///< Time-to-live for cache entries in milliseconds.
    std::unordered_map<Key, CacheEntry> cache_map_;  ///< The map that stores the cached entries.
    std::list<Key> access_order_;  ///< The list that tracks the access order for LRU eviction.
    LoggerCallback logger_;  ///< Optional callback for logging cache operations.
    EvictionCallback eviction_callback_;  ///< Optional callback for eviction operations.
    mutable std::shared_mutex mutex_;  ///< Mutex to ensure thread-safety for cache operations.

    /**
     * @brief Checks if the given cache entry has expired based on its timestamp.
     *
     * @param timestamp The timestamp of the cache entry to check.
     * @return true if the entry has expired, false otherwise.
     */
    bool is_expired(const std::chrono::steady_clock::time_point &timestamp) const {
        return std::chrono::steady_clock::now() - timestamp > std::chrono::milliseconds(ttl_millis_);
    }

    /**
     * @brief Updates the access order list for the LRU eviction policy.
     *
     * This method moves the accessed key to the end of the list to mark it as the most recently used.
     *
     * @param key The key to update in the access order.
     */
    void update_access_order(const Key &key) {
        access_order_.remove(key);
        access_order_.push_back(key);
    }

    /**
     * @brief Adds a new entry to the cache.
     *
     * This method inserts a new key-value pair into the cache, and updates the access order.
     *
     * @param key The key to insert.
     * @param value The value to associate with the key.
     */
    void add_new_entry(const Key &key, const Value &value) {
        cache_map_[key] = {value, std::chrono::steady_clock::now()};
        access_order_.push_back(key);
        log("PUT: " + key_to_string(key));
    }

    /**
     * @brief Updates an existing cache entry.
     *
     * This method updates the value and timestamp of an existing entry, and updates its position in the access order.
     *
     * @param it The iterator pointing to the existing entry in the cache.
     * @param value The new value to store.
     */
    void update_existing_entry(typename std::unordered_map<Key, CacheEntry>::iterator it, const Value &value) {
        it->second.value = value;
        it->second.timestamp = std::chrono::steady_clock::now();
        update_access_order(it->first);
        log("UPDATE: " + key_to_string(it->first));
    }

    /**
     * @brief Converts the key to a string for logging purposes.
     *
     * This method provides a string representation of the key, handling both string and non-string key types.
     *
     * @param key The key to convert to a string.
     * @return A string representation of the key.
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
     * @brief Logs cache operations if a logger callback is provided.
     *
     * @param message The log message to output.
     */
    void log(const std::string &message) const {
        if (logger_) logger_(message);
    }

};
