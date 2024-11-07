cmake_minimum_required(VERSION 3.10)

project(Distributed_KV_Store_Master VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB UTIL_SOURCES "util/*.cpp")

add_executable(MyExecutable main.cpp
        ${UTIL_SOURCES}
        handleClient/clientMainFunction.cpp
        handleHeartbeat/heartbeatMainFunction.cpp
        handleLocalCommand/localCommandMainFunction.cpp)

include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.10.5
)
FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(MyExecutable PRIVATE nlohmann_json::nlohmann_json)

include_directories(
        ${CMAKE_SOURCE_DIR}/util
        ${CMAKE_SOURCE_DIR}/handleClient
        ${CMAKE_SOURCE_DIR}/handleHeartbeat
        ${CMAKE_SOURCE_DIR}/handleLocalCommand
)