cmake_minimum_required(VERSION 3.16)
project(FingerPrintProcessor LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler optimizations for Linux GCC
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx2 -mfma -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")

# Add threading support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Find required packages
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/utils/Timer.cpp
    src/core/FileManager.cpp
    src/core/CorePointDetector.cpp
    src/core/FeatureExtractor.cpp
    src/core/AddressGenerator.cpp
    src/database/DatabaseWriter.cpp
)

# Create executable
add_executable(fingerprint_processor ${SOURCES})

# Link libraries
target_link_libraries(fingerprint_processor 
    ${OpenCV_LIBS} 
    ${SQLITE3_LIBRARIES}
    pthread
)

# Compiler-specific definitions
target_compile_definitions(fingerprint_processor PRIVATE
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Debug>:DEBUG>
)

# Set output directory
set_target_properties(fingerprint_processor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
