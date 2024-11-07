//
// Created by Shawn Wan on 2024/11/6.
//
#ifndef KV_STORE_MAP_H
#define KV_STORE_MAP_H

#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <iostream>

class KVStoreMap {
private:
    std::unordered_map<std::string, std::string> store;
    mutable std::shared_mutex mutex;

public:
    void write(const std::string& key, const std::string& value);

    std::string read(const std::string& key) const;
};

#endif // KV_STORE_MAP_H