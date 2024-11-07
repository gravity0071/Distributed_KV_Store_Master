//
// Created by Shawn Wan on 2024/11/6.
//

#include "ConsistentHashingMap.h"

void ConsistentHashingMap::write(int key, const std::string& value) {
    std::unique_lock lock(mutex);
    map[key] = value;
    std::cout << "ConsistentHashingMap: Wrote " << key << " = " << value << std::endl;
}

std::string ConsistentHashingMap::read(int key) const {
    std::shared_lock lock(mutex);
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return {};
}