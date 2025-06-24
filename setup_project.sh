#!/bin/bash

# Create project directory structure
mkdir -p kvstore/{include,src,tests,build}
cd kvstore

# Create CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.12)
project(KVStore VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include)

set(KVSTORE_SOURCES
    src/lru_cache.cpp
    src/kvstore.cpp
)

add_library(kvstore_lib STATIC ${KVSTORE_SOURCES})

add_executable(kvstore_cli src/cli.cpp)
target_link_libraries(kvstore_cli kvstore_lib pthread)

add_executable(kvstore_benchmark src/benchmark.cpp)
target_link_libraries(kvstore_benchmark kvstore_lib pthread)

enable_testing()

find_package(GTest QUIET)
if(GTest_FOUND)
    add_executable(kvstore_tests tests/test_kvstore.cpp)
    target_link_libraries(kvstore_tests kvstore_lib GTest::gtest GTest::gtest_main pthread)
    add_test(NAME KVStoreTests COMMAND kvstore_tests)
else()
    message(WARNING "Google Test not found. Tests will not be built.")
endif()

install(TARGETS kvstore_cli kvstore_benchmark RUNTIME DESTINATION bin)
install(FILES include/kvstore.h DESTINATION include)
install(TARGETS kvstore_lib ARCHIVE DESTINATION lib)
EOF

# Create README.md
cat > README.md << 'EOF'
# KVStore - High-Performance In-Memory Key-Value Store

A Redis-like key-value store implemented in C++17 with LRU eviction, snapshots, and concurrency control.

## Features

- **LRU Cache**: O(1) get/put operations with automatic eviction
- **Thread Safety**: Multi-reader, single-writer concurrency
- **Persistence**: Binary snapshots with fast recovery
- **Performance Metrics**: Real-time statistics and benchmarking
- **CLI Interface**: Redis-like command interface

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
