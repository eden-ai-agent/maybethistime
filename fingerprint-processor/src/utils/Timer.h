// Timer.h - High-precision timing 
// Implementation placeholder 
#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>

/**
 * High-precision timing utility for fingerprint processing
 * Optimized for Linux/GitHub Codespaces environment
 */
class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double, std::micro>; // microseconds

private:
    TimePoint start_time;
    bool is_running;
    
    // Static profiling data
    static std::mutex profile_mutex;
    static std::unordered_map<std::string, double> total_times;
    static std::unordered_map<std::string, int> call_counts;

public:
    Timer() : is_running(false) {}

    // Start timing
    void start() {
        start_time = Clock::now();
        is_running = true;
    }

    // Stop timing and return elapsed microseconds
    double stop() {
        if (!is_running) return 0.0;
        
        auto end_time = Clock::now();
        is_running = false;
        
        Duration elapsed = end_time - start_time;
        return elapsed.count();
    }

    // Get elapsed time without stopping
    double elapsed() const {
        if (!is_running) return 0.0;
        
        auto current_time = Clock::now();
        Duration elapsed = current_time - start_time;
        return elapsed.count();
    }

    // Restart timer (stop and start)
    double restart() {
        double elapsed_time = stop();
        start();
        return elapsed_time;
    }

    // Static utility functions for easy timing
    static double time_function(std::function<void()> func) {
        Timer timer;
        timer.start();
        func();
        return timer.stop();
    }

    // Profiling methods
    static void profile_start(const std::string& name);
    static double profile_stop(const std::string& name);
    static void profile_add(const std::string& name, double time_us);
    static void print_profile_summary();
    static void clear_profile_data();
    
    // Convenience functions for common time conversions
    static double microseconds_to_milliseconds(double us) { return us / 1000.0; }
    static double microseconds_to_seconds(double us) { return us / 1000000.0; }
    static double milliseconds_to_microseconds(double ms) { return ms * 1000.0; }
    static double seconds_to_microseconds(double s) { return s * 1000000.0; }

    // Format timing results
    static std::string format_time(double microseconds) {
        if (microseconds < 1000.0) {
            return std::to_string(static_cast<int>(microseconds)) + "μs";
        } else if (microseconds < 1000000.0) {
            return std::to_string(microseconds / 1000.0) + "ms";
        } else {
            return std::to_string(microseconds / 1000000.0) + "s";
        }
    }
};

// RAII timing helper class
class ScopedTimer {
private:
    Timer timer;
    std::string name;
    bool profile;

public:
    explicit ScopedTimer(const std::string& timer_name = "", bool enable_profiling = false) 
        : name(timer_name), profile(enable_profiling) {
        timer.start();
        if (profile && !name.empty()) {
            Timer::profile_start(name);
        }
    }

    ~ScopedTimer() {
        double elapsed = timer.stop();
        if (profile && !name.empty()) {
            Timer::profile_stop(name);
        }
        if (!name.empty()) {
            // Could log timing here if needed
        }
    }

    double elapsed() const {
        return timer.elapsed();
    }
};
