#!/bin/bash

# KVStore Build Script
set -e

echo "Building KVStore..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the project
make -j$(nproc)

echo "Build completed successfully!"
echo ""
echo "Available executables:"
echo "  ./kvstore_cli        - Interactive CLI"
echo "  ./kvstore_benchmark  - Performance benchmarks"
if [ -f "./kvstore_tests" ]; then
    echo "  ./kvstore_tests      - Unit tests"
fi

echo ""
echo "Quick start:"
echo "  ./kvstore_cli --capacity 10000"
echo "  ./kvstore_benchmark --threads 4 --operations 10000"
