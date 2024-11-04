cmake_minimum_required(VERSION 3.10)

project(Distributed_KV_Store_Master VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 4. 添加可执行文件
add_executable(MyExecutable main.cpp)