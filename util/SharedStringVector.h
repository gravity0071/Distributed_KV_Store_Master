//
// Created by Shawn Wan on 2024/11/6.
//
#ifndef SHARED_STRING_VECTOR_H
#define SHARED_STRING_VECTOR_H

#include <vector>
#include <string>
#include <shared_mutex>
#include <iostream>

class SharedStringVector {
private:
    std::vector<std::string> vec;
    mutable std::shared_mutex mutex;

public:
    void add(const std::string& value);
    std::vector<std::string> getAll() const;
};

#endif // SHARED_STRING_VECTOR_H