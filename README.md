# CCache - A C++ LRU Cache with TTL Support

## Overview

**CCache** is a generic, thread-safe, time-to-live (TTL) cache implementation in C++. It supports:
1. Least Recently Used (LRU) eviction policy.
2. Time-to-Live (TTL) for cache entries.
3. Logging and custom eviction callbacks.
4. Thread-safe operations using a `std::shared_mutex`.

The cache can be used to store key-value pairs of arbitrary types and efficiently manage limited resources in scenarios such as caching database queries, API responses, or expensive computations.

---

## Features

- **Configurable Capacity**: Set a maximum number of entries the cache can hold.
- **TTL for Expiry**: Automatically evict entries after a configurable time-to-live.
- **Eviction Policy**: Evicts the least recently used item when capacity is exceeded.
- **Thread Safety**: Uses `std::shared_mutex` for safe access in concurrent environments.
- **Custom Callbacks**: Supports optional logging and eviction callback functions.

---

## API Documentation

| **Method**              | **Description**                                                                                   | **Time Complexity**               |
|-------------------------|---------------------------------------------------------------------------------------------------|-----------------------------------|
| `contains`              | Checks if the cache contains a key.                                                               | O(1)                              |
| `get`                   | Retrieves the value associated with a key. If expired, evicts the key.                            | O(1)                              |
| `put`                   | Adds or updates a key-value pair. Evicts the least recently used (LRU) item if the cache is full. | O(1)                              |
| `evict_lru`             | Evicts the least recently used item.                                                              | O(1)                              |
| `evict`                 | Evicts a specific key from the cache.                                                             | O(1)                              |
| `with_cache`            | Retrieves a cached value or computes and caches it if absent.                                     | O(1) for retrieval or computation |
| **Private Methods**     |                                                                                                   |                                   |
| `is_expired`            | Checks if a cache entry is expired based on its timestamp.                                        | O(1)                              |
| `update_access_order`   | Updates access order to maintain LRU tracking.                                                    | O(N) for linked list              |
| `add_new_entry`         | Adds a new entry to the cache and updates metadata.                                               | O(1)                              |
| `update_existing_entry` | Updates an existing cache entry and refreshes its TTL.                                            | O(N) for linked list              |

---

## Constructor

```cpp
explicit CCache(const int max_size = 100, 
                const long long ttl_millis = 1000 * 60 * 60 * 5,
                LoggerCallback logger = nullptr, 
                EvictionCallback eviction_callback = nullptr);
```

### Parameters
- **`max_size`**: Maximum number of entries the cache can hold. Must be greater than 0.
- **`ttl_millis`**: Time-to-live for cache entries in milliseconds. Must be greater than 0.
- **`logger`** *(optional)*: Function for logging cache operations. Accepts a `std::string` message.
- **`eviction_callback`** *(optional)*: Function invoked when an entry is evicted. Accepts the key and value of the evicted entry.

---

## Example Usage

```cpp
#include "CCache.h"
#include <iostream>

int main() {

    auto logger = [](const std::string &message) { std::cout << message << std::endl; };
    auto eviction_callback = [](const std::string &key, const std::string &value) {
        std::cout << "Evicted: " << key << " -> " << value << std::endl;
    };

    CCache<std::string, std::string> cache(3, 5000, logger, eviction_callback);

    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");
    
    std::cout << "Contains key2: " << cache.contains("key2") << std::endl;

    cache.get("key1");
    cache.put("key4", "value4"); // Evicts "key2" as it is LRU.

    cache.with_cache("key5", [] { return "computed_value"; });
    
    return 0;
}
```

---

## Detailed Explanation

### Internal Data Structures

- **`std::unordered_map<Key, CacheEntry>`**: Maps keys to their values and metadata.
- **`std::list<Key>`**: Maintains the access order for LRU policy.

### Cache Entry

A `CacheEntry` contains:
- **`value`**: The cached value.
- **`timestamp`**: A `std::chrono::steady_clock::time_point` representing the last update time.

---

## Thread Safety

The cache uses `std::shared_mutex`:
- **`std::shared_lock`** for read operations like `contains`.
- **`std::unique_lock`** for write operations like `put` and `evict`.

### Performance Note
To optimize LRU updates, the `std::list` is used to manage access order. Operations like moving elements to the back take O(N) in the worst case.

---

## TTL and Eviction

1. **Time-To-Live (TTL)**:
    - Each entry has a timestamp.
    - If the current time exceeds the timestamp + TTL, the entry is considered expired and is evicted.

2. **LRU Policy**:
    - When the cache reaches its maximum size, the least recently used (first in `std::list`) entry is evicted.

---

## Logging and Callbacks

- **LoggerCallback**:
  A function to log messages about cache operations.

- **EvictionCallback**:
  A function triggered when an entry is evicted, providing insights into which keys are being removed.

---

## Visualization

### Workflow Diagram

```plaintext
  +---------+      +-------------+      +-------------+
  |   PUT   | ---> | Evict LRU?  | ---> | Add Entry   |
  +---------+      +-------------+      +-------------+
                          |
                          v
                   +-------------+
                   |  Evict LRU  |
                   +-------------+

  +---------+      +-------------+      +-------------+
  |   GET   | ---> | Expired?    | ---> | Return Value |
  +---------+      +-------------+      +-------------+
                          |
                          v
                   +-------------+
                   |   Evict     |
                   +-------------+
```

### Memory Representation

| **Key** | **Value** | **Timestamp**      | **Access Order**   |
|---------|-----------|--------------------|--------------------|
| `key1`  | `value1`  | `2024-11-22 10:00` | `key3, key1, key4` |
| `key4`  | `value4`  | `2024-11-22 10:02` |                    |
| `key3`  | `value3`  | `2024-11-22 10:01` |                    |

---

## Performance Analysis

| Operation    | Best Case | Worst Case | Description                                  |
|--------------|-----------|------------|----------------------------------------------|
| `get`        | O(1)      | O(1)       | Direct lookup in the hash map.               |
| `put`        | O(1)      | O(N)       | Eviction may involve removing LRU from list. |
| `evict_lru`  | O(1)      | O(1)       | Direct removal from hash map and list.       |
| `with_cache` | O(1)      | O(N)       | Combines `get` and optional computation.     |

---

## Future Improvements

- **Customizable TTL per entry**.
- **Serialization and Deserialization** for persistence.
- **Async Support** for non-blocking cache operations.

