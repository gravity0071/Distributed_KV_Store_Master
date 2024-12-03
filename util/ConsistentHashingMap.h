#ifndef DISTRIBUTED_KV_STORE_MASTER_CONSISTENTHASHINGMAP_H
#define DISTRIBUTED_KV_STORE_MASTER_CONSISTENTHASHINGMAP_H

#include <string>
#include <map>
#include <shared_mutex>
#include "HashCalculator.h"
#define HASH_KEY_RANGE 10000

// key range:0 - 10000
// should be assigned like: [0 - 10001)

class ConsistentHashingMap {
private:
    std::map<std::string, std::string> hashMap; // key: "start-end", value: store_id
    mutable std::shared_mutex mutex;           // Shared mutex for thread safety
    HashCalculator hashCalculator;             // Hash calculator instance

public:
    // Helper function to parse range keys
    bool parseRange(const std::string& rangeKey, size_t& start, size_t& end) const;
    // Constructor: Allows configuration of hash range
    explicit ConsistentHashingMap(size_t hashRange = HASH_KEY_RANGE) : hashCalculator(hashRange) {}
    auto begin() { return hashMap.begin(); }
    auto end() { return hashMap.end(); }

    // Find the store_id responsible for a given key
    std::string findParticularKey(const std::string& key) const;

    // Remove an old range and assign it to a new range
    int removeRange(const std::string& oldRange, const std::string& newRange);

    // Add a new store and assign it to a new range
    //if the map is empty, return the whole key range
    int addNew(const std::string& oldRange, const std::string& newRange, const std::string& storeId);

    // Debug: Display the entire hash ring
    void displayHashRing() const;
};

#endif // DISTRIBUTED_KV_STORE_MASTER_CONSISTENTHASHINGMAP_H