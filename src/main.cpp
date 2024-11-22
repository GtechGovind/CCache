#include <iostream>
#include <thread>

#include "CCache.h"

// A simple logger callback function that prints log messages to the console.
void logger(const std::string &message) {
    std::cout << "[LOG]: " << message << std::endl;
}

// A simple eviction callback function that prints out the evicted key and value.
void eviction_callback(const std::string &key, const int &value) {
    std::cout << "[EVICTED]: Key = " << key << ", Value = " << value << std::endl;
}

int main() {

    // Initialize the cache with:
    // - max_size = 3 (the cache can hold at most 3 items before evicting the least recently used)
    // - ttl_millis = 5000 (items will expire after 5 seconds)
    // - logger function to log cache operations
    // - eviction callback to print evicted items
    CCache<std::string, int> cache(3, 5000, logger, eviction_callback);

    // 1. Add items to the cache
    std::cout << "Adding items to the cache..." << std::endl;
    cache.put("key1", 100);  // Adds key1 with value 100
    cache.put("key2", 200);  // Adds key2 with value 200
    cache.put("key3", 300);  // Adds key3 with value 300

    // 2. Retrieve items from the cache
    std::cout << "Retrieving items from the cache..." << std::endl;
    if (const auto value1 = cache.get("key1")) {
        std::cout << "Key1: " << *value1 << std::endl;  // Expected output: 100
    } else {
        std::cout << "Key1 not found in the cache." << std::endl;
    }

    // Check for key2 and key3
    if (const auto value2 = cache.get("key2")) {
        std::cout << "Key2: " << *value2 << std::endl;  // Expected output: 200
    } else {
        std::cout << "Key2 not found in the cache." << std::endl;
    }

    if (const auto value3 = cache.get("key3")) {
        std::cout << "Key3: " << *value3 << std::endl;  // Expected output: 300
    } else {
        std::cout << "Key3 not found in the cache." << std::endl;
    }

    // 3. Trigger eviction: Add a 4th item to exceed the cache size (max_size = 3)
    std::cout << "Adding a 4th item to trigger eviction..." << std::endl;
    cache.put("key4", 400);  // This will evict the least recently used item (key1)

    // Check the cache state after eviction
    std::cout << "Final state of the cache after eviction:" << std::endl;

    // Verify that key1 has been evicted (LRU eviction)
    if (const auto value = cache.get("key1")) {
        std::cout << "Key1: " << *value << std::endl;
    } else {
        std::cout << "Key1 has been evicted!" << std::endl;  // Expected output: evicted
    }

    // Check key2, key3, and key4 after eviction
    if (const auto value = cache.get("key2")) {
        std::cout << "Key2: " << *value << std::endl;  // Expected output: 200
    } else {
        std::cout << "Key2 evicted!" << std::endl;
    }

    if (const auto value = cache.get("key3")) {
        std::cout << "Key3: " << *value << std::endl;  // Expected output: 300
    } else {
        std::cout << "Key3 evicted!" << std::endl;
    }

    if (const auto value = cache.get("key4")) {
        std::cout << "Key4: " << *value << std::endl;  // Expected output: 400
    } else {
        std::cout << "Key4 evicted!" << std::endl;
    }

    // 4. Simulate TTL expiration
    std::cout << "Accessing key2 again after TTL expires..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(6000)); // Sleep for 6 seconds, more than TTL (5000ms)

    // After TTL expires, key2 should be evicted
    if (const auto ttl_expired_value = cache.get("key2")) {
        std::cout << "Key2 (TTL expired): " << *ttl_expired_value << std::endl;
    } else {
        std::cout << "Key2 expired and evicted!" << std::endl;  // Expected output: expired
    }

    // 5. Demonstrating `with_cache`: Fetch value or compute if absent
    std::cout << "Using 'with_cache' to fetch or compute a value..." << std::endl;
    const auto computed_value = cache.with_cache("key5", []() {
        std::cout << "Computing value for key5..." << std::endl;
        return 500;  // Simulate an expensive computation for the value of key5
    });

    // Display the computed value
    std::cout << "Key5: " << computed_value << std::endl;  // Expected output: 500

    // 6. Using `with_cache` to check if a key is available or cache a computed value
    std::cout << "Using 'with_cache' for an already cached key (key3)..." << std::endl;
    const auto cached_value = cache.with_cache("key3", []() {
        std::cout << "This computation won't happen because key3 is already cached." << std::endl;
        return 999;  // This value won't be computed as key3 is already in cache
    });

    // Show the cached value of key3
    std::cout << "Key3: " << cached_value << std::endl;  // Expected output: 300 (cached value)

    return 0;

}
