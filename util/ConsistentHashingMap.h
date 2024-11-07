//
// Created by Shawn Wan on 2024/11/6.
//
#ifndef CONSISTENT_HASHING_MAP_H
#define CONSISTENT_HASHING_MAP_H

#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <iostream>

class ConsistentHashingMap {
private:
    std::unordered_map<int, std::string> map;
    mutable std::shared_mutex mutex;

public:
    void write(int key, const std::string& value);
    std::string read(int key) const;
};

#endif // CONSISTENT_HASHING_MAP_H