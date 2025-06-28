// FileManager.cpp - FileManager implementation 
// Implementation placeholder 
#include "FileManager.h"
#include "../utils/Logger.h"
#include <filesystem>
#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

// Static member definitions
std::unordered_map<std::string, FileManager::ImageCache> FileManager::image_cache;
std::mutex FileManager::cache_mutex;
size_t FileManager::max_cache_size_mb = 256; // 256MB default
size_t FileManager::current_cache_size = 0;

const std::vector<std::string> FileManager::supported_extensions = {
    ".bmp", ".jpg", ".jpeg", ".png", ".tiff", ".tif", ".gif"
};

// Cache statistics
static size_t cache_hits = 0;
static size_t cache_misses = 0;

bool FileManager::is_supported_extension(const std::string& filepath) {
    std::string ext = get_file_extension(filepath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return std::find(supported_extensions.begin(), supported_extensions.end(), ext) 
           != supported_extensions.end();
}

std::string FileManager::get_file_extension(const std::string& filepath) {
    size_t pos = filepath.find_last_of('.');
    if (pos == std::string::npos) return "";
    return filepath.substr(pos);
}

std::string FileManager::get_filename_from_path(const std::string& filepath) {
    return fs::path(filepath).filename().string();
}

size_t FileManager::calculate_image_memory_size(const cv::Mat& image) {
    return image.total() * image.elemSize();
}

void FileManager::cleanup_cache_if_needed() {
    const size_t max_cache_bytes = max_cache_size_mb * 1024 * 1024;
    
    while (current_cache_size > max_cache_bytes && !image_cache.empty()) {
        remove_oldest_cache_entry();
    }
}

void FileManager::remove_oldest_cache_entry() {
    if (image_cache.empty()) return;
    
    auto oldest_it = image_cache.begin();
    for (auto it = image_cache.begin(); it != image_cache.end(); ++it) {
        if (it->second.last_accessed < oldest_it->second.last_accessed) {
            oldest_it = it;
        }
    }
    
    current_cache_size -= oldest_it->second.memory_size;
    Logger::debug("Removed from cache: " + oldest_it->first);
    image_cache.erase(oldest_it);
}

std::vector<FileManager::FileInfo> FileManager::scan_directory(const std::string& directory_path, 
                                                             bool recursive) {
    std::vector<FileInfo> files;
    
    if (!directory_exists(directory_path)) {
        Logger::error("Directory does not exist: " + directory_path);
        return files;
    }
    
    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(directory_path)) {
                if (entry.is_regular_file() && is_supported_extension(entry.path().string())) {
                    files.push_back(get_file_info(entry.path().string()));
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(directory_path)) {
                if (entry.is_regular_file() && is_supported_extension(entry.path().string())) {
                    files.push_back(get_file_info(entry.path().string()));
                }
            }
        }
        
        Logger::info("Found " + std::to_string(files.size()) + " supported image files in " + directory_path);
        
    } catch (const fs::filesystem_error& e) {
        Logger::error("Filesystem error scanning directory: " + std::string(e.what()));
    }
    
    return files;
}

FileManager::FileInfo FileManager::get_file_info(const std::string& filepath) {
    FileInfo info;
    info.filepath = normalize_path(filepath);
    info.filename = get_filename_from_path(filepath);
    info.is_valid = false;
    
    if (!file_exists(filepath)) {
        info.error_message = "File does not exist";
        return info;
    }
    
    if (!is_supported_extension(filepath)) {
        info.error_message = "Unsupported file extension";
        return info;
    }
    
    info.file_size = get_file_size(filepath);
    info.is_valid = true;
    
    return info;
}

cv::Mat FileManager::load_image(const std::string& filepath, bool use_cache) {
    std::string normalized_path = normalize_path(filepath);
    
    // Check cache first
    if (use_cache) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = image_cache.find(normalized_path);
        if (it != image_cache.end()) {
            it->second.last_accessed = std::chrono::steady_clock::now();
            cache_hits++;
            Logger::debug("Cache hit: " + normalized_path);
            return it->second.image.clone(); // Return copy for thread safety
        }
        cache_misses++;
    }
    
    // Load image from disk
    cv::Mat image = cv::imread(normalized_path, cv::IMREAD_GRAYSCALE);
    
    if (image.empty()) {
        Logger::error("Failed to load image: " + normalized_path);
        return cv::Mat();
    }
    
    // Validate image
    if (!is_valid_fingerprint_image(image)) {
        Logger::warning("Image may not be suitable for fingerprint processing: " + normalized_path);
    }
    
    // Add to cache if requested
    if (use_cache) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        
        ImageCache cache_entry;
        cache_entry.image = image.clone();
        cache_entry.filepath = normalized_path;
        cache_entry.memory_size = calculate_image_memory_size(image);
        cache_entry.last_accessed = std::chrono::steady_clock::now();
        
        current_cache_size += cache_entry.memory_size;
        image_cache[normalized_path] = std::move(cache_entry);
        
        Logger::debug("Added to cache: " + normalized_path);
        cleanup_cache_if_needed();
    }
    
    return image;
}

bool FileManager::validate_image(const cv::Mat& image) {
    return !image.empty() && image.channels() == 1; // Grayscale only
}

std::vector<cv::Mat> FileManager::load_images_batch(const std::vector<std::string>& filepaths,
                                                   bool use_cache,
                                                   std::vector<bool>* success_flags) {
    std::vector<cv::Mat> images;
    images.reserve(filepaths.size());
    
    if (success_flags) {
        success_flags->clear();
        success_flags->reserve(filepaths.size());
    }
    
    for (const auto& filepath : filepaths) {
        cv::Mat image = load_image(filepath, use_cache);
        images.push_back(image);
        
        if (success_flags) {
            success_flags->push_back(!image.empty());
        }
    }
    
    return images;
}

void FileManager::clear_cache() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    image_cache.clear();
    current_cache_size = 0;
    Logger::info("Image cache cleared");
}

void FileManager::remove_from_cache(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(cache_mutex);
    std::string normalized_path = normalize_path(filepath);
    
    auto it = image_cache.find(normalized_path);
    if (it != image_cache.end()) {
        current_cache_size -= it->second.memory_size;
        image_cache.erase(it);
        Logger::debug("Removed from cache: " + normalized_path);
    }
}

std::vector<std::string> FileManager::get_cached_files() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    std::vector<std::string> files;
    files.reserve(image_cache.size());
    
    for (const auto& pair : image_cache) {
        files.push_back(pair.first);
    }
    
    return files;
}

bool FileManager::file_exists(const std::string& filepath) {
    return fs::exists(filepath) && fs::is_regular_file(filepath);
}

bool FileManager::directory_exists(const std::string& directory_path) {
    return fs::exists(directory_path) && fs::is_directory(directory_path);
}

bool FileManager::create_directory(const std::string& directory_path) {
    try {
        return fs::create_directories(directory_path);
    } catch (const fs::filesystem_error& e) {
        Logger::error("Failed to create directory: " + std::string(e.what()));
        return false;
    }
}

size_t FileManager::get_file_size(const std::string& filepath) {
    try {
        return fs::file_size(filepath);
    } catch (const fs::filesystem_error&) {
        return 0;
    }
}

bool FileManager::is_valid_fingerprint_image(const cv::Mat& image) {
    if (image.empty()) return false;
    
    // Basic checks for fingerprint images
    if (image.channels() != 1) return false; // Must be grayscale
    if (image.rows < 100 || image.cols < 100) return false; // Minimum size
    if (image.rows > 2000 || image.cols > 2000) return false; // Maximum reasonable size
    
    // Check if image has reasonable contrast (not all black or all white)
    cv::Scalar mean, stddev;
    cv::meanStdDev(image, mean, stddev);
    
    return stddev[0] > 10.0; // Minimum standard deviation for contrast
}

std::string FileManager::validate_image_for_processing(const cv::Mat& image) {
    if (image.empty()) return "Image is empty";
    if (image.channels() != 1) return "Image must be grayscale";
    if (image.rows < 101 || image.cols < 101) return "Image too small (minimum 101x101)";
    
    cv::Scalar mean, stddev;
    cv::meanStdDev(image, mean, stddev);
    if (stddev[0] < 5.0) return "Image has insufficient contrast";
    
    return ""; // Empty string means valid
}

std::string FileManager::normalize_path(const std::string& path) {
    return fs::path(path).lexically_normal().string();
}

std::string FileManager::get_directory_from_path(const std::string& filepath) {
    return fs::path(filepath).parent_path().string();
}

std::string FileManager::combine_paths(const std::string& dir, const std::string& filename) {
    return (fs::path(dir) / filename).string();
}

FileManager::CacheStats FileManager::get_cache_statistics() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    CacheStats stats;
    stats.total_entries = image_cache.size();
    stats.total_memory_mb = current_cache_size / (1024 * 1024);
    stats.cache_hits = cache_hits;
    stats.cache_misses = cache_misses;
    
    size_t total_requests = cache_hits + cache_misses;
    stats.hit_ratio = total_requests > 0 ? static_cast<double>(cache_hits) / total_requests : 0.0;
    
    return stats;
}

void FileManager::reset_cache_statistics() {
    cache_hits = 0;
    cache_misses = 0;
}

// FileBatch implementation
FileBatch::FileBatch(const std::string& directory_path, bool recursive) : current_index(0) {
    files = FileManager::scan_directory(directory_path, recursive);
}

FileBatch::FileBatch(const std::vector<std::string>& filepaths) : current_index(0) {
    files.reserve(filepaths.size());
    for (const auto& filepath : filepaths) {
        files.push_back(FileManager::get_file_info(filepath));
    }
}

FileManager::FileInfo FileBatch::next() {
    if (!has_next()) {
        throw std::out_of_range("No more files in batch");
    }
    return files[current_index++];
}
