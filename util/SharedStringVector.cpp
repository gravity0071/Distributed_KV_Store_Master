//
// Created by Shawn Wan on 2024/11/6.
//

#include "SharedStringVector.h"

void SharedStringVector::add(const std::string& value) {
    std::unique_lock lock(mutex);
    vec.push_back(value);
//    std::cout << "SharedStringVector: Added " << value << std::endl;
}

std::vector<std::string> SharedStringVector::getAll() const {
    std::shared_lock lock(mutex);
    return vec;
}

bool SharedStringVector::have(const std::string& val){
    std::shared_lock lock(mutex);
    auto it = std::find(vec.begin(), vec.end(), val);
    if(it != vec.end())
        return true;
    else
        return false;
}


bool SharedStringVector::remove(const std::string& value) {
    std::unique_lock lock(mutex);
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it != vec.end()) {
        vec.erase(it); // Remove the element if found
//        std::cout << "SharedStringVector: Removed " << value << std::endl;
    } else {
        std::cerr << "SharedStringVector: Value " << value << " not found" << std::endl;
    }
    return true;
}