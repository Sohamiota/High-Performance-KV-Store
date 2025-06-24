# KVStore - High-Performance In-Memory Key-Value Store

A Redis-like key-value store implemented in C++17 with LRU eviction, snapshots, and concurrency control.

## Features

- **LRU Cache**: O(1) get/put operations with automatic eviction
- **Thread Safety**: Multi-reader, single-writer concurrency
- **Persistence**: Binary snapshots with fast recovery
- **Performance Metrics**: Real-time statistics and benchmarking
- **CLI Interface**: Redis-like command interface

## Building

\`\`\`bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
\`\`\`

## Usage

\`\`\`bash
# Start CLI
./kvstore_cli --capacity 10000

# Run benchmarks  
./kvstore_benchmark --threads 8 --operations 50000

# Run tests (if Google Test is available)
./kvstore_tests
\`\`\`

## CLI Commands

- `GET <key>` - Get value for key
- `PUT <key> <value>` - Set key to value
- `DEL <key>` - Delete key
- `CLEAR` - Clear all entries
- `SIZE` - Show number of entries
- `STATS` - Show performance statistics
- `SAVE` - Save snapshot to disk
- `LOAD` - Load snapshot from disk
- `HELP` - Show available commands
- `QUIT` - Exit the program

## Dependencies

- C++17 compiler (GCC 7+ or Clang 5+)
- CMake 3.12+
- pthread
- Google Test (optional, for tests)

### Installing Dependencies

\`\`\`bash
# Ubuntu/Debian
sudo apt install build-essential cmake libgtest-dev

# macOS
brew install cmake googletest

# CentOS/RHEL
sudo yum install gcc-c++ cmake gtest-devel
\`\`\`

## Performance

The KVStore is designed for high performance:
- O(1) average case for get/put operations
- Thread-safe concurrent access
- Memory-efficient LRU eviction
- Fast binary snapshots
- Optimized for low-latency workloads

## Architecture

- **LRU Cache**: Hash map + doubly linked list for O(1) operations
- **Concurrency**: Reader-writer locks for multi-threaded access
- **Persistence**: Custom binary format with memory-mapped files
- **Metrics**: Real-time performance tracking
\`\`\`

```cmake file="CMakeLists.txt"
cmake_minimum_required(VERSION 3.12)
project(KVStore VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(include)

# Source files
set(KVSTORE_SOURCES
    src/lru_cache.cpp
    src/kvstore.cpp
)

# Create static library
add_library(kvstore_lib STATIC ${KVSTORE_SOURCES})

# CLI executable
add_executable(kvstore_cli src/cli.cpp)
target_link_libraries(kvstore_cli kvstore_lib pthread)

# Benchmark executable
add_executable(kvstore_benchmark src/benchmark.cpp)
target_link_libraries(kvstore_benchmark kvstore_lib pthread)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest QUIET)
if(GTest_FOUND)
    add_executable(kvstore_tests tests/test_kvstore.cpp)
    target_link_libraries(kvstore_tests kvstore_lib GTest::gtest GTest::gtest_main pthread)
    add_test(NAME KVStoreTests COMMAND kvstore_tests)
else()
    message(WARNING "Google Test not found. Tests will not be built.")
endif()

# Installation
install(TARGETS kvstore_cli kvstore_benchmark
        RUNTIME DESTINATION bin)

install(FILES include/kvstore.h
        DESTINATION include)

install(TARGETS kvstore_lib
        ARCHIVE DESTINATION lib)

# Package configuration
set(CPACK_PACKAGE_NAME "KVStore")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-Performance In-Memory Key-Value Store")
include(CPack)
