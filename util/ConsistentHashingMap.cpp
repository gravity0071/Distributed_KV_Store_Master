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

    // Iterate through the hashMap to find the responsible range
    for (const auto &[rangeKey, storeId] : hashMap) {
        size_t start, end;
        if (parseRange(rangeKey, start, end)) {
            if ((start <= end && hashValue >= start && hashValue < end) || // Standard range
                (start > end && (hashValue >= start || hashValue < end))) { // Wraparound range
                return storeId;
            }
        }
    }

    return ""; // Key not found in any range
}

// Remove an old range and assign it to a new range
int ConsistentHashingMap::removeRange(const std::string &oldRange, const std::string &newRange) {
    std::unique_lock lock(mutex); // Exclusive lock for writing

    auto old_it = hashMap.find(oldRange);
    if (old_it == hashMap.end()) {
        return 0; // Old range not found
    }

    size_t old_start, old_end, new_start, new_end;
    if (!parseRange(oldRange, old_start, old_end) || !parseRange(newRange, new_start, new_end)) {
        return 0; // Invalid range format
    }

    std::string storeId = old_it->second;

    // Remove the old range
    hashMap.erase(old_it);

    // Handle combined ranges
    size_t combined_start = std::min(old_start, new_start);
    size_t combined_end = std::max(old_end, new_end);

    // Add the new combined range
    std::string combinedRange = std::to_string(combined_start) + "-" + std::to_string(combined_end);
    hashMap[combinedRange] = storeId;

    std::cout << "Reassigned range from " << oldRange << " to " << combinedRange
              << " for store: " << storeId << std::endl;
    return 1;
}

// Add a new store and assign it to a new range
int ConsistentHashingMap::addNew(const std::string &oldRange, const std::string &newRange, const std::string &storeId) {
    std::unique_lock lock(mutex); // Exclusive lock for writing

    auto old_it = hashMap.find(oldRange);
    if (old_it == hashMap.end()) {
        return 0; // Old range not found
    }

    size_t old_start, old_end, new_start, new_end;
    if (!parseRange(oldRange, old_start, old_end) || !parseRange(newRange, new_start, new_end)) {
        return 0; // Invalid range format
    }

    if (new_start < old_start || new_end > old_end) {
        return 0; // New range is not a subset of the old range
    }

    // Adjust the old range to exclude the new range
    hashMap.erase(old_it); // Remove the old range

    if (new_start > old_start) {
        std::string adjustedRange1 = std::to_string(old_start) + "-" + std::to_string(new_start);
        hashMap[adjustedRange1] = old_it->second;
    }

    if (new_end < old_end) {
        std::string adjustedRange2 = std::to_string(new_end) + "-" + std::to_string(old_end);
        hashMap[adjustedRange2] = old_it->second;
    }

    // Add the new range for the new store
    hashMap[newRange] = storeId;

    std::cout << "Added new range: " << newRange << " for store: " << storeId << std::endl;
    return 1;
}

// Debug: Display the entire hash ring
void ConsistentHashingMap::displayHashRing() const {
    std::shared_lock lock(mutex); // Shared lock for reading

    std::cout << "Hash Ring:" << std::endl;
    for (const auto &[rangeKey, storeId] : hashMap) {
        std::cout << "Range: " << rangeKey << " -> Store ID: " << storeId << std::endl;
    }
}