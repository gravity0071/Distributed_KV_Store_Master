#ifndef DISTRIBUTED_KV_STORE_MASTER_CONSISTENTHASHINGMAP_H
#define DISTRIBUTED_KV_STORE_MASTER_CONSISTENTHASHINGMAP_H

#include <string>
#include <map>
#include <shared_mutex>
#include "HashCalculator.h"

class ConsistentHashingMap {
private:
    std::map<std::string, std::string> hashMap; // key: "start-end", value: store_id
    mutable std::shared_mutex mutex;           // Shared mutex for thread safety
    HashCalculator hashCalculator;             // Hash calculator instance

    // Helper function to parse range keys
    bool parseRange(const std::string& rangeKey, size_t& start, size_t& end) const;

public:
    // Constructor: Allows configuration of hash range
    explicit ConsistentHashingMap(size_t hashRange = 10000) : hashCalculator(hashRange) {}

    // Find the store_id responsible for a given key
    std::string findParticularKey(const std::string& key) const;

    // Remove an old range and assign it to a new range
    int removeRange(const std::string& oldRange, const std::string& newRange);

    // Add a new store and assign it to a new range
    int addNew(const std::string& oldRange, const std::string& newRange, const std::string& storeId);

    // Debug: Display the entire hash ring
    void displayHashRing() const;
};

#endif // DISTRIBUTED_KV_STORE_MASTER_CONSISTENTHASHINGMAP_H