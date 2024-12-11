#include "ConsistentHashingMap.h"
#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>

// Helper: Parse a range key ("start-end") into start and end values
bool ConsistentHashingMap::parseRange(const std::string &rangeKey, size_t &start, size_t &end) const {
    std::istringstream rangeStream(rangeKey);
    char dash;
    rangeStream >> start >> dash >> end;
    return rangeStream && dash == '-';
}

// Find the store_id responsible for a given key
std::string ConsistentHashingMap::findParticularKey(const std::string &key) const {
    std::shared_lock lock(mutex); // Shared lock for reading

    size_t hashValue = hashCalculator.calculateHash(key);
//    std::cout << "currently looking for: " <<  hashValue << std::endl;

    // Iterate through the hashMap to find the responsible range
    for (const auto &[rangeKey, storeId] : hashMap) {
        size_t start, end;
        if (parseRange(rangeKey, start, end)) {
            if(start == end) //only have one store and it contains all the possible keys
                return storeId;
            if ((start < end && hashValue >= start && hashValue < end) || // Standard range
                (start > end && (hashValue >= start || hashValue < end))) { // Wraparound range
                return storeId;
            }
        }
    }

    return ""; // Key not found in any range
}

// Add a new store and assign it to a new range
int ConsistentHashingMap::addNew(const std::string &oldRange, const std::string &newRange, const std::string &storeId) {
    std::unique_lock lock(mutex); // Exclusive lock for writing

    if (hashMap.empty()) {
        std::string fullRange = "0-" + std::to_string(HASH_KEY_RANGE);
        hashMap[fullRange] = storeId; // Assign the full range to the store
//        std::cout << "Map is empty. Assigned the full range: " << fullRange << " to store: " << storeId << std::endl;
        return HASH_KEY_RANGE; // Return the hash key range
    }

//    std::cout << "addNew: input: oldRange:" << oldRange <<std::endl;
//    std::cout << "addNew: input: newRange:" << newRange <<std::endl;

    auto old_it = hashMap.find(oldRange);
    if (old_it == hashMap.end()) {
        return 0; // Old range not found
    }

//    std::cout << "addNew: find old store id: " << old_it->second <<std::endl;

    size_t oldStart, oldEnd, newStart, newEnd;
    if (!parseRange(oldRange, oldStart, oldEnd) || !parseRange(newRange, newStart, newEnd)) {
        return 0; // Invalid range format
    }

    // Validate the new range: one boundary must match
    if (!(newStart == oldStart || newEnd == oldEnd)) {
        return 0; // New range must share a boundary with the old range
    }

    std::string old_id = old_it->second;
    // Remove the old range
    hashMap.erase(old_it);

    // Adjust the old range to exclude the new range
    if (newStart % HASH_KEY_RANGE == oldStart % HASH_KEY_RANGE) {
        // New range starts where the old range starts
        hashMap[std::to_string(newEnd) + "-" + std::to_string(oldEnd)] = old_id;
    } else if (newEnd % HASH_KEY_RANGE == oldEnd % HASH_KEY_RANGE) {
        // New range ends where the old range ends
        hashMap[std::to_string(oldStart) + "-" + std::to_string(newStart)] = old_id;
    } else {
        return 0; // Invalid range adjustment
    }

    // Add the new range for the new store
    hashMap[newRange] = storeId;

    std::cout << "Added new range: " << newRange << " for store: " << storeId << std::endl;
    return 1;
}

// Remove an old range and assign it to a new range, new range should be adjacent to the old range
int ConsistentHashingMap::removeRange(const std::string &oldRange, const std::string &newRange) {
    std::unique_lock lock(mutex); // Exclusive lock for writing

    // Find the old and new ranges in the map
    auto old_it = hashMap.find(oldRange);
    auto new_it = hashMap.find(newRange);

    if (old_it == hashMap.end() || new_it == hashMap.end()) {
        return 0; // Old range or new range not found
    }

    size_t oldStart, oldEnd, newStart, newEnd;
    if (!parseRange(oldRange, oldStart, oldEnd) || !parseRange(newRange, newStart, newEnd)) {
        return 0; // Invalid range format
    }

    // Check adjacency or wraparound
    bool isAdjacent = (oldEnd == newStart) || (oldStart == newEnd);
    size_t combinedStart, combinedEnd;
    if(oldEnd == newStart || (oldEnd == HASH_KEY_RANGE && (newStart == 0 || newStart == HASH_KEY_RANGE))){
        combinedStart = oldStart;
        combinedEnd = newEnd;
    }else{
        combinedStart = newStart;
        combinedEnd = oldEnd;
    }
    std::string storeId = new_it->second; // Retain the store ID from the old range

    // Remove old and new ranges from the map
    hashMap.erase(old_it);
    hashMap.erase(new_it);

    // Add the combined range
    std::string combinedRange = std::to_string(combinedStart % HASH_KEY_RANGE) + "-" + std::to_string(combinedEnd % HASH_KEY_RANGE);
    auto result = hashMap.emplace(combinedRange, storeId);

    if (!result.second) {
        return 0; // Failed to insert the combined range
    }

    std::cout << "Connected ranges " << oldRange << " and " << newRange
              << " into " << combinedRange << " for store: " << storeId << std::endl;
    return 1;
}

// Debug: Display the entire hash ring
void ConsistentHashingMap::displayHashRing() const {
    std::shared_lock lock(mutex); // Shared lock for reading

    std::cout << "Hash Ring:" << std::endl;
    for (const auto &[rangeKey, storeId] : hashMap) {
        std::cout << "Key range: " << rangeKey << " -> Store Id: " << storeId << std::endl;
    }
}