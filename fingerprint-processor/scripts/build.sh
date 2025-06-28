#!/bin/bash 
# Linux compilation script 
#!/bin/bash

# Fingerprint Processor Build Script for Linux
echo "Building Fingerprint Processor..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
    echo "Created build/ directory"
fi

# Navigate to build directory
cd build

# Configure with CMake (Release build for speed)
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Check if CMake configuration succeeded
if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building project..."
make -j$(nproc)

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "Executable: build/bin/fingerprint_processor"
    
    # Show executable info
    if [ -f "bin/fingerprint_processor" ]; then
        echo "File size: $(du -h bin/fingerprint_processor | cut -f1)"
        echo "To run: ./build/bin/fingerprint_processor"
    fi
else
    echo "❌ Build failed!"
    exit 1
fi
