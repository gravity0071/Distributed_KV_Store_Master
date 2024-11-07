//
// Created by Shawn Wan on 2024/11/6.
//
// KVStoreMap.cpp

#include "KVStoreMap.h"
#include <iostream>
#include <map>

// Function to write a key-value pair to the store
void KVStoreMap::write(const std::string& key, const std::string& value) {
    std::unique_lock lock(mutex);  // Exclusive lock for writing
    store[key] = value;
    std::cout << "KVStoreMap: Wrote " << key << " = " << value << std::endl;
}

// Function to read a value based on a key from the store
std::optional<std::string> KVStoreMap::read(const std::string& key) const {
    std::unique_lock lock(mutex);  // Exclusive lock for reading
    auto it = store.find(key);
    if (it != store.end()) {
        return it->second;
    }
    return std::nullopt;  // Indicate the key was not found
}

// Internal helper function to update a specific field in the JSON data
void KVStoreMap::updateField(const std::string& key, const std::string& field, const std::string& value) {
    std::unique_lock lock(mutex);
    auto it = store.find(key);

    std::map<std::string, std::string> jsonData;
    if (it != store.end()) {
        jsonData = jsonParser.JsonToMap(it->second);
    }
    jsonData[field] = value;
    store[key] = jsonParser.MapToJson(jsonData);
}

// Getter function to retrieve the IP address associated with a key
std::string KVStoreMap::getIp(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) {
        return "";  // Or handle the missing key case as needed
    }

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("ip") != jsonData.end()) {
        return jsonData["ip"];
    }
    return "";  // Return an empty string if "ip" key does not exist
}

// Getter function for the heartbeat port associated with a key
std::string KVStoreMap::getHeartbeatPort(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) return "";

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("heartbeatPort") != jsonData.end()) {
        return jsonData["heartbeatPort"];
    }
    return "";
}

// Getter function for the add/drop port associated with a key
std::string KVStoreMap::getAddDropPort(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) return "";

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("addDropPort") != jsonData.end()) {
        return jsonData["addDropPort"];
    }
    return "";
}

// Getter function for the client port associated with a key
std::string KVStoreMap::getClientPort(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) return "";

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("clientPort") != jsonData.end()) {
        return jsonData["clientPort"];
    }
    return "";
}

// Getter function to check the store status associated with a key
std::string KVStoreMap::getStoreStatus(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) return "";

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("alive") != jsonData.end()) {
        return jsonData["alive"];
    }
    return "";
}

// Getter function for the number of keys in the store associated with a key
std::string KVStoreMap::getStoreKeyNum(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) return "";

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("keyNum") != jsonData.end()) {
        return jsonData["keyNum"];
    }
    return "";
}

// Getter function for the key range associated with a key
std::string KVStoreMap::getKeyRange(const std::string& key) const {
    auto data = read(key);
    if (!data.has_value()) return "";

    auto jsonData = jsonParser.JsonToMap(data.value());
    if (jsonData.find("keyRange") != jsonData.end()) {
        return jsonData["keyRange"];
    }
    return "";
}

void KVStoreMap::setIp(const std::string& key, const std::string& ip) {
    updateField(key, "ip", ip);
}

void KVStoreMap::setHeartbeatPort(const std::string& key, const std::string& port) {
    updateField(key, "heartbeatPort", port);
}

void KVStoreMap::setAddDropPort(const std::string& key, const std::string& port) {
    updateField(key, "addDropPort", port);
}

void KVStoreMap::setClientPort(const std::string& key, const std::string& port) {
    updateField(key, "clientPort", port);
}

void KVStoreMap::setStoreStatus(const std::string& key, const std::string& status) {
    updateField(key, "alive", status);
}

void KVStoreMap::setStoreKeyNum(const std::string& key, const std::string& keyNum) {
    updateField(key, "keyNum", keyNum);
}

void KVStoreMap::setKeyRange(const std::string& key, const std::string& keyRange) {
    updateField(key, "keyRange", keyRange);
}

// Implementation of the setAllFields function
void KVStoreMap::setAllFields(const std::string& key, const std::string& ip, const std::string& heartbeatPort,
                              const std::string& addDropPort, const std::string& clientPort, const std::string& status,
                              const std::string& keyNum, const std::string& keyRange) {
    std::unique_lock lock(mutex);

    // Create an ordered map for JSON fields to maintain consistent order
    std::map<std::string, std::string> jsonData;
    jsonData["ip"] = ip;
    jsonData["heartbeatPort"] = heartbeatPort;
    jsonData["addDropPort"] = addDropPort;
    jsonData["clientPort"] = clientPort;
    jsonData["alive"] = status;
    jsonData["keyNum"] = keyNum;
    jsonData["keyRange"] = keyRange;

    // Write the JSON string directly to the store
    store[key] = jsonParser.MapToJson(jsonData);
}