//
// Created by Shawn Wan on 2024/11/6.
//

#include "SharedStringVector.h"

void SharedStringVector::add(const std::string& value) {
    std::unique_lock lock(mutex);
    vec.push_back(value);
    std::cout << "SharedStringVector: Added " << value << std::endl;
}

std::vector<std::string> SharedStringVector::getAll() const {
    std::shared_lock lock(mutex);
    return vec;
}