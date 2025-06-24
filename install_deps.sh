#!/bin/bash

# Dependency installation script for different platforms

echo "Installing KVStore dependencies..."

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if command -v apt-get &> /dev/null; then
        # Ubuntu/Debian
        echo "Installing dependencies for Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y build-essential cmake libgtest-dev
        
        # Build and install Google Test if needed
        if [ ! -f "/usr/lib/libgtest.a" ]; then
            cd /usr/src/gtest
            sudo cmake CMakeLists.txt
            sudo make
            sudo cp lib/*.a /usr/lib
        fi
        
    elif command -v yum &> /dev/null; then
        # CentOS/RHEL
        echo "Installing dependencies for CentOS/RHEL..."
        sudo yum groupinstall -y "Development Tools"
        sudo yum install -y cmake gtest-devel
        
    elif command -v dnf &> /dev/null; then
        # Fedora
        echo "Installing dependencies for Fedora..."
        sudo dnf groupinstall -y "Development Tools"
        sudo dnf install -y cmake gtest-devel
    fi
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Installing dependencies for macOS..."
    if command -v brew &> /dev/null; then
        brew install cmake googletest
    else
        echo "Please install Homebrew first: https://brew.sh"
        exit 1
    fi
    
else
    echo "Unsupported operating system: $OSTYPE"
    echo "Please install the following manually:"
    echo "  - C++17 compiler (GCC 7+ or Clang 5+)"
    echo "  - CMake 3.12+"
    echo "  - Google Test (optional)"
    exit 1
fi

echo "Dependencies installed successfully!"
echo "You can now build the project with: ./build.sh"
