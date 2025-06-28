// Timer.cpp - Timer implementation 
// Implementation placeholder 
#include "Timer.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// Static member definitions
std::mutex Timer::profile_mutex;
std::unordered_map<std::string, double> Timer::total_times;
std::unordered_map<std::string, int> Timer::call_counts;

// Thread-local storage for nested profiling
thread_local std::unordered_map<std::string, Timer> active_timers;

void Timer::profile_start(const std::string& name) {
    active_timers[name].start();
}

double Timer::profile_stop(const std::string& name) {
    auto it = active_timers.find(name);
    if (it == active_timers.end()) {
        return 0.0;
    }
    
    double elapsed = it->second.stop();
    
    // Add to global profiling data
    {
        std::lock_guard<std::mutex> lock(profile_mutex);
        total_times[name] += elapsed;
        call_counts[name]++;
    }
    
    active_timers.erase(it);
    return elapsed;
}

void Timer::profile_add(const std::string& name, double time_us) {
    std::lock_guard<std::mutex> lock(profile_mutex);
    total_times[name] += time_us;
    call_counts[name]++;
}

void Timer::print_profile_summary() {
    std::lock_guard<std::mutex> lock(profile_mutex);
    
    if (total_times.empty()) {
        std::cout << "No profiling data available." << std::endl;
        return;
    }

    std::cout << "\n=== PROFILING SUMMARY ===" << std::endl;
    std::cout << std::left << std::setw(25) << "Function"
              << std::right << std::setw(10) << "Calls"
              << std::setw(15) << "Total (μs)"
              << std::setw(15) << "Avg (μs)"
              << std::setw(15) << "Total (ms)"
              << std::setw(15) << "Avg (ms)" << std::endl;
    std::cout << std::string(95, '-') << std::endl;

    // Sort by total time (descending)
    std::vector<std::pair<std::string, double>> sorted_times;
    for (const auto& pair : total_times) {
        sorted_times.emplace_back(pair.first, pair.second);
    }
    std::sort(sorted_times.begin(), sorted_times.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& pair : sorted_times) {
        const std::string& name = pair.first;
        double total_us = pair.second;
        int calls = call_counts[name];
        double avg_us = total_us / calls;
        double total_ms = total_us / 1000.0;
        double avg_ms = avg_us / 1000.0;

        std::cout << std::left << std::setw(25) << name
                  << std::right << std::setw(10) << calls
                  << std::setw(15) << std::fixed << std::setprecision(1) << total_us
                  << std::setw(15) << std::fixed << std::setprecision(1) << avg_us
                  << std::setw(15) << std::fixed << std::setprecision(3) << total_ms
                  << std::setw(15) << std::fixed << std::setprecision(3) << avg_ms
                  << std::endl;
    }

    std::cout << std::string(95, '-') << std::endl;
    
    // Calculate total processing time
    double grand_total = 0.0;
    for (const auto& pair : total_times) {
        grand_total += pair.second;
    }
    
    std::cout << "Total processing time: " << format_time(grand_total) << std::endl;
    std::cout << "========================" << std::endl;
}

void Timer::clear_profile_data() {
    std::lock_guard<std::mutex> lock(profile_mutex);
    total_times.clear();
    call_counts.clear();
}
