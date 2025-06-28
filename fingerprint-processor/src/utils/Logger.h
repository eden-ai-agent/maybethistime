#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * Thread-safe logging utility for fingerprint processing
 * Optimized for Linux/GitHub Codespaces environment
 */
class Logger {
public:
    enum class Level {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

private:
    static std::mutex log_mutex;
    static Level current_level;
    static std::ofstream log_file;
    static bool console_output;
    static bool file_output;

    static std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    static std::string level_to_string(Level level) {
        switch (level) {
            case Level::DEBUG:   return "DEBUG";
            case Level::INFO:    return "INFO ";
            case Level::WARNING: return "WARN ";
            case Level::ERROR:   return "ERROR";
            default:             return "UNKN ";
        }
    }

    static void write_log(Level level, const std::string& message) {
        if (level < current_level) return;

        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::string timestamp = get_timestamp();
        std::string level_str = level_to_string(level);
        std::string full_message = "[" + timestamp + "] [" + level_str + "] " + message;

        // Console output
        if (console_output) {
            if (level >= Level::ERROR) {
                std::cerr << full_message << std::endl;
            } else {
                std::cout << full_message << std::endl;
            }
        }

        // File output
        if (file_output && log_file.is_open()) {
            log_file << full_message << std::endl;
            log_file.flush(); // Ensure immediate write for debugging
        }
    }

public:
    // Initialize logging system
    static void init(const std::string& log_filename = "", 
                    Level level = Level::INFO,
                    bool enable_console = true,
                    bool enable_file = false) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        current_level = level;
        console_output = enable_console;
        file_output = enable_file;

        if (enable_file && !log_filename.empty()) {
            log_file.open(log_filename, std::ios::app);
            if (!log_file.is_open()) {
                std::cerr << "Failed to open log file: " << log_filename << std::endl;
                file_output = false;
            }
        }
    }

    // Clean shutdown
    static void shutdown() {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    // Logging methods
    static void debug(const std::string& message) {
        write_log(Level::DEBUG, message);
    }

    static void info(const std::string& message) {
        write_log(Level::INFO, message);
    }

    static void warning(const std::string& message) {
        write_log(Level::WARNING, message);
    }

    static void error(const std::string& message) {
        write_log(Level::ERROR, message);
    }

    // Template versions for easy formatting
    template<typename... Args>
    static void debug(const std::string& format, Args... args) {
        debug(format_string(format, args...));
    }

    template<typename... Args>
    static void info(const std::string& format, Args... args) {
        info(format_string(format, args...));
    }

    template<typename... Args>
    static void warning(const std::string& format, Args... args) {
        warning(format_string(format, args...));
    }

    template<typename... Args>
    static void error(const std::string& format, Args... args) {
        error(format_string(format, args...));
    }

    // Set logging level dynamically
    static void set_level(Level level) {
        std::lock_guard<std::mutex> lock(log_mutex);
        current_level = level;
    }

    static Level get_level() {
        std::lock_guard<std::mutex> lock(log_mutex);
        return current_level;
    }

private:
    // Simple string formatting helper
    template<typename... Args>
    static std::string format_string(const std::string& format, Args... args) {
        // Simple implementation - could be enhanced with proper formatting
        std::stringstream ss;
        ss << format;
        ((ss << " " << args), ...);
        return ss.str();
    }
};

// Static member definitions
std::mutex Logger::log_mutex;
Logger::Level Logger::current_level = Logger::Level::INFO;
std::ofstream Logger::log_file;
bool Logger::console_output = true;
bool Logger::file_output = false;
