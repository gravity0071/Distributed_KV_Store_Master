//
// Created by Shawn Wan on 2024/11/6.
//

#include "KVStoreMap.h"

void KVStoreMap::write(const std::string& key, const std::string& value) {
    std::unique_lock lock(mutex);
    store[key] = value;
    std::cout << "KVStoreMap: Wrote " << key << " = " << value << std::endl;
}

std::string KVStoreMap::read(const std::string& key) const {
    std::shared_lock lock(mutex);
    auto it = store.find(key);
    if (it != store.end()) {
        return it->second;
    }
    return {};
}