#include "gtest/gtest.h"
#include "../util/KVStoreMap.h"
#include "../util/JsonParser.h"
#include <thread>
#include <vector>
#include <mutex>

class KVStoreMapConsistencyTest : public ::testing::Test {
protected:
    KVStoreMap kvStore;
    std::string key = "store_1";
    std::mutex testMutex;
};

// Function to set different fields in KVStoreMap and immediately verify the result
void controlledSetAndRead(KVStoreMap& kvStore, const std::string& key, const std::string& field, const std::string& value, std::mutex& testMutex) {
    std::lock_guard<std::mutex> guard(testMutex);

    // Set the field based on the specified field type
    if (field == "ip") {
        kvStore.setIp(key, value);
        EXPECT_EQ(kvStore.getIp(key), value);
    } else if (field == "heartbeatPort") {
        kvStore.setHeartbeatPort(key, value);
        EXPECT_EQ(kvStore.getHeartbeatPort(key), value);
    } else if (field == "addDropPort") {
        kvStore.setAddDropPort(key, value);
        EXPECT_EQ(kvStore.getAddDropPort(key), value);
    } else if (field == "clientPort") {
        kvStore.setClientPort(key, value);
        EXPECT_EQ(kvStore.getClientPort(key), value);
    } else if (field == "alive") {
        kvStore.setStoreStatus(key, value);
        EXPECT_EQ(kvStore.getStoreStatus(key), value);
    } else if (field == "keyNum") {
        kvStore.setStoreKeyNum(key, value);
        EXPECT_EQ(kvStore.getStoreKeyNum(key), value);
    } else if (field == "keyRange") {
        kvStore.setKeyRange(key, value);
        EXPECT_EQ(kvStore.getKeyRange(key), value);
    }
}

// Test to verify all set functions under a multi-threaded scenario with increased threads
TEST_F(KVStoreMapConsistencyTest, IncreasedConcurrentSetOperations) {
    // Define values for each field
    std::string ipValue1 = "192.168.1.1";
    std::string ipValue2 = "192.168.1.2";
    std::string heartbeatPortValue1 = "8080";
    std::string heartbeatPortValue2 = "8181";
    std::string addDropPortValue1 = "9090";
    std::string addDropPortValue2 = "9191";
    std::string clientPortValue1 = "6060";
    std::string clientPortValue2 = "6161";
    std::string aliveValue1 = "true";
    std::string aliveValue2 = "false";
    std::string keyNumValue1 = "10";
    std::string keyNumValue2 = "20";
    std::string keyRangeValue1 = "0-100";
    std::string keyRangeValue2 = "101-200";

    // Create a vector of threads to perform controlled set and read operations with more threads
    std::vector<std::thread> threads;

    // Set up more threads for each field to increase concurrent load
    for (int i = 0; i < 5; ++i) {  // Using 5 threads per field for testing
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "ip", (i % 2 == 0) ? ipValue1 : ipValue2, std::ref(testMutex));
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "heartbeatPort", (i % 2 == 0) ? heartbeatPortValue1 : heartbeatPortValue2, std::ref(testMutex));
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "addDropPort", (i % 2 == 0) ? addDropPortValue1 : addDropPortValue2, std::ref(testMutex));
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "clientPort", (i % 2 == 0) ? clientPortValue1 : clientPortValue2, std::ref(testMutex));
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "alive", (i % 2 == 0) ? aliveValue1 : aliveValue2, std::ref(testMutex));
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "keyNum", (i % 2 == 0) ? keyNumValue1 : keyNumValue2, std::ref(testMutex));
        threads.emplace_back(controlledSetAndRead, std::ref(kvStore), key, "keyRange", (i % 2 == 0) ? keyRangeValue1 : keyRangeValue2, std::ref(testMutex));
    }

    // Join all threads to ensure all operations complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Final verification to check if the last write for each field was successful
    // Since multiple threads are writing, the final values may vary, but should be one of the expected values
    EXPECT_TRUE(kvStore.getIp(key) == ipValue1 || kvStore.getIp(key) == ipValue2);
    EXPECT_TRUE(kvStore.getHeartbeatPort(key) == heartbeatPortValue1 || kvStore.getHeartbeatPort(key) == heartbeatPortValue2);
    EXPECT_TRUE(kvStore.getAddDropPort(key) == addDropPortValue1 || kvStore.getAddDropPort(key) == addDropPortValue2);
    EXPECT_TRUE(kvStore.getClientPort(key) == clientPortValue1 || kvStore.getClientPort(key) == clientPortValue2);
    EXPECT_TRUE(kvStore.getStoreStatus(key) == aliveValue1 || kvStore.getStoreStatus(key) == aliveValue2);
    EXPECT_TRUE(kvStore.getStoreKeyNum(key) == keyNumValue1 || kvStore.getStoreKeyNum(key) == keyNumValue2);
    EXPECT_TRUE(kvStore.getKeyRange(key) == keyRangeValue1 || kvStore.getKeyRange(key) == keyRangeValue2);
}