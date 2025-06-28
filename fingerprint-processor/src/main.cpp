#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>

// Linux-specific includes
#include <unistd.h>
#include <sys/resource.h>

// Project includes
#include "utils/Logger.h"
#include "utils/Timer.h"
#include "core/FileManager.h"
#include "core/CorePointDetector.h"

namespace fs = std::filesystem;

// Test configuration
struct TestConfig {
    std::string input_directory = "test_data";
    std::string output_directory = "output";
    bool verbose = false;
    int max_files = -1; // -1 means process all files
};

// Print system information for debugging
void printSystemInfo() {
    Logger::info("=== System Information ===");
    
    // Get CPU info
    Logger::info("CPU Cores: " + std::to_string(sysconf(_SC_NPROCESSORS_ONLN)));
    
    // Get memory info
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        Logger::info("Peak Memory: " + std::to_string(usage.ru_maxrss / 1024) + " MB");
    }
    
    // Check for AVX2 support
    #ifdef __AVX2__
    Logger::info("AVX2 Support: Enabled");
    #else
    Logger::info("AVX2 Support: Disabled (using scalar fallback)");
    #endif
    
    Logger::info("Build Type: " 
    #ifdef NDEBUG
        "Release"
    #else
        "Debug"
    #endif
    );
}

// Process a single fingerprint image
bool processSingleImage(const std::string& filepath, 
                       FileManager& fileManager,
                       CorePointDetector& detector) {
    Timer timer;
    timer.start();
    
    Logger::info("Processing: " + fs::path(filepath).filename().string());
    
    // Step 1: Load image
    timer.start();
    cv::Mat image = fileManager.loadImage(filepath);
    if (image.empty()) {
        Logger::error("Failed to load image: " + filepath);
        return false;
    }
    auto loadTime = timer.getElapsedMicroseconds();
    
    Logger::info("  Loaded: " + std::to_string(image.cols) + "x" + 
                std::to_string(image.rows) + " in " + 
                std::to_string(loadTime) + "μs");
    
    // Step 2: Detect core points
    timer.start();
    std::vector<CorePoint> corePoints = detector.detectCorePoints(image);
    auto detectionTime = timer.getElapsedMicroseconds();
    
    Logger::info("  Core points found: " + std::to_string(corePoints.size()) + 
                " in " + std::to_string(detectionTime) + "μs");
    
    // Step 3: Extract ROIs (regions of interest)
    timer.start();
    std::vector<ROI> rois;
    for (size_t i = 0; i < corePoints.size(); ++i) {
        ROI roi = detector.extractROI(image, corePoints[i], 
                                    fs::path(filepath).filename().string(), 
                                    static_cast<int32_t>(i));
        rois.push_back(roi);
    }
    auto roiTime = timer.getElapsedMicroseconds();
    
    Logger::info("  ROIs extracted: " + std::to_string(rois.size()) + 
                " in " + std::to_string(roiTime) + "μs");
    
    auto totalTime = loadTime + detectionTime + roiTime;
    Logger::info("  Total time: " + std::to_string(totalTime) + "μs");
    
    return true;
}

// Batch process multiple images
void batchProcessImages(const TestConfig& config) {
    Logger::info("=== Starting Batch Processing ===");
    
    // Initialize components
    FileManager fileManager;
    CorePointDetector detector;
    
    // Find all image files in input directory
    std::vector<std::string> imageFiles;
    
    if (!fs::exists(config.input_directory)) {
        Logger::error("Input directory does not exist: " + config.input_directory);
        return;
    }
    
    for (const auto& entry : fs::directory_iterator(config.input_directory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            // Convert to lowercase for comparison
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".bmp" || ext == ".jpg" || ext == ".jpeg" || 
                ext == ".png" || ext == ".tiff" || ext == ".tif") {
                imageFiles.push_back(entry.path().string());
            }
        }
    }
    
    if (imageFiles.empty()) {
        Logger::error("No image files found in: " + config.input_directory);
        return;
    }
    
    // Limit number of files if specified
    if (config.max_files > 0 && imageFiles.size() > static_cast<size_t>(config.max_files)) {
        imageFiles.resize(config.max_files);
    }
    
    Logger::info("Found " + std::to_string(imageFiles.size()) + " image files");
    
    // Process each image
    Timer batchTimer;
    batchTimer.start();
    
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& filepath : imageFiles) {
        if (processSingleImage(filepath, fileManager, detector)) {
            successCount++;
        } else {
            failCount++;
        }
    }
    
    auto totalBatchTime = batchTimer.getElapsedMicroseconds();
    
    // Print summary
    Logger::info("=== Batch Processing Complete ===");
    Logger::info("Successful: " + std::to_string(successCount));
    Logger::info("Failed: " + std::to_string(failCount));
    Logger::info("Total time: " + std::to_string(totalBatchTime / 1000) + "ms");
    
    if (successCount > 0) {
        auto avgTime = totalBatchTime / successCount;
        Logger::info("Average per image: " + std::to_string(avgTime) + "μs");
        
        double imagesPerSecond = 1000000.0 / avgTime;
        Logger::info("Processing rate: " + std::to_string(imagesPerSecond) + " images/second");
    }
}

// Print usage information
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -i <dir>     Input directory (default: test_data)\n";
    std::cout << "  -o <dir>     Output directory (default: output)\n";
    std::cout << "  -n <count>   Max files to process (default: all)\n";
    std::cout << "  -v           Verbose output\n";
    std::cout << "  -h           Show this help\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << programName << " -i test_data -n 10 -v\n";
}

int main(int argc, char* argv[]) {
    TestConfig config;
    
    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "i:o:n:vh")) != -1) {
        switch (opt) {
            case 'i':
                config.input_directory = optarg;
                break;
            case 'o':
                config.output_directory = optarg;
                break;
            case 'n':
                config.max_files = std::atoi(optarg);
                break;
            case 'v':
                config.verbose = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    // Initialize logging
    if (config.verbose) {
        Logger::setLevel(Logger::Level::DEBUG);
    }
    
    Logger::info("Fingerprint Processor Starting...");
    
    // Print system information
    printSystemInfo();
    
    // Create output directory if it doesn't exist
    if (!fs::exists(config.output_directory)) {
        fs::create_directories(config.output_directory);
        Logger::info("Created output directory: " + config.output_directory);
    }
    
    // Run batch processing
    batchProcessImages(config);
    
    Logger::info("Program completed successfully");
    return 0;
}
