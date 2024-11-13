#include "gtest/gtest.h"
#include "../util/ConsistentHashingMap.h"
#include <thread>
#include <vector>
#include <mutex>

class ConsistentHashingMapTest : public ::testing::Test {
protected:
    ConsistentHashingMap hashMap; // ConsistentHashingMap instance
};

// Function to add or remove ranges concurrently
void concurrentAddOrRemove(ConsistentHashingMap& hashMap, const std::string& oldRange, const std::string& newRange,
                           const std::string& storeId, bool isAddOperation) {
    if (isAddOperation) {
        hashMap.addNew(oldRange, newRange, storeId);
    } else {
        hashMap.removeRange(oldRange, newRange);
    }
}

// Test to verify the addNew function
TEST_F(ConsistentHashingMapTest, AddNewRange) {
    EXPECT_EQ(hashMap.addNew("0-10001", "0-5000", "store_1"), 10000);
    EXPECT_EQ(hashMap.addNew("0-10001", "5000-10001", "store_2"), 1);
    hashMap.displayHashRing();
}

// Test to verify the removeRange function
TEST_F(ConsistentHashingMapTest, RemoveRange) {
    hashMap.addNew("0-10001", "0-5000", "store_1");
    hashMap.addNew("0-10001", "5000-10001", "store_2");
    EXPECT_EQ(hashMap.removeRange("0-5000", "5000-10001"), 1);
    hashMap.displayHashRing();
}

// Test to verify concurrent add and remove operations
TEST_F(ConsistentHashingMapTest, ConcurrentOperations) {
    // Initial range setup
    hashMap.addNew("0-10001", "0-5000", "store_1");
    hashMap.addNew("0-10001", "5000-10001", "store_2");

    std::vector<std::thread> threads;

    // Perform concurrent additions and removals
    threads.emplace_back(concurrentAddOrRemove, std::ref(hashMap), "0-5000", "2500-5000", "store_3", true);  // Add
    threads.emplace_back(concurrentAddOrRemove, std::ref(hashMap), "5000-10001", "7500-10001", "store_4", true); // Add
    threads.emplace_back(concurrentAddOrRemove, std::ref(hashMap), "2500-5000", "0-2500", "", false);         // Remove
    threads.emplace_back(concurrentAddOrRemove, std::ref(hashMap), "7500-10001", "5000-7500", "", false);     // Remove

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify the final state of the hash ring
    hashMap.displayHashRing();
}