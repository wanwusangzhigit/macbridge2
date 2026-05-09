#include "utils/diagnostics.h"
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif

namespace WinDarling {

// PerformanceTimer 实现
PerformanceTimer::PerformanceTimer() 
    : mName("unnamed"), mIsRunning(false) {
}

PerformanceTimer::PerformanceTimer(const std::string& name) 
    : mName(name), mIsRunning(false) {
}

PerformanceTimer::~PerformanceTimer() {
    if (mIsRunning) {
        stop();
    }
}

void PerformanceTimer::start() {
    mStart = std::chrono::high_resolution_clock::now();
    mIsRunning = true;
}

void PerformanceTimer::stop() {
    if (mIsRunning) {
        mEnd = std::chrono::high_resolution_clock::now();
        mIsRunning = false;
    }
}

void PerformanceTimer::reset() {
    mIsRunning = false;
}

double PerformanceTimer::getElapsedSeconds() const {
    auto duration = mIsRunning 
        ? std::chrono::high_resolution_clock::now() - mStart 
        : mEnd - mStart;
    return std::chrono::duration<double>(duration).count();
}

double PerformanceTimer::getElapsedMilliseconds() const {
    auto duration = mIsRunning 
        ? std::chrono::high_resolution_clock::now() - mStart 
        : mEnd - mStart;
    return std::chrono::duration<double, std::milli>(duration).count();
}

double PerformanceTimer::getElapsedMicroseconds() const {
    auto duration = mIsRunning 
        ? std::chrono::high_resolution_clock::now() - mStart 
        : mEnd - mStart;
    return std::chrono::duration<double, std::micro>(duration).count();
}

// PerformanceProfiler 实现
PerformanceProfiler& PerformanceProfiler::getInstance() {
    static PerformanceProfiler instance;
    return instance;
}

PerformanceProfiler::PerformanceProfiler() {
}

PerformanceProfiler::~PerformanceProfiler() {
}

void PerformanceProfiler::startTiming(const std::string& name) {
    mActiveTimers[name].start();
}

void PerformanceProfiler::stopTiming(const std::string& name) {
    auto it = mActiveTimers.find(name);
    if (it != mActiveTimers.end()) {
        it->second.stop();
        double elapsed = it->second.getElapsedMilliseconds();
        
        auto& metric = mMetrics[name];
        metric.name = name;
        metric.callCount++;
        metric.totalTimeMs += elapsed;
        metric.avgTimeMs = metric.totalTimeMs / metric.callCount;
        metric.minTimeMs = (std::min)(metric.minTimeMs, elapsed);
        metric.maxTimeMs = (std::max)(metric.maxTimeMs, elapsed);
        
        mActiveTimers.erase(it);
    }
}

void PerformanceProfiler::reset(const std::string& name) {
    mMetrics.erase(name);
    mActiveTimers.erase(name);
}

void PerformanceProfiler::resetAll() {
    mMetrics.clear();
    mActiveTimers.clear();
}

void PerformanceProfiler::printReport(std::ostream& out) const {
    out << "========================================\n";
    out << "  Performance Profiler Report\n";
    out << "========================================\n";
    
    if (mMetrics.empty()) {
        out << "No metrics available.\n";
        return;
    }
    
    out << std::left << std::setw(25) << "Name" 
        << std::setw(10) << "Count" 
        << std::setw(12) << "Total(ms)" 
        << std::setw(12) << "Avg(ms)" 
        << std::setw(12) << "Min(ms)" 
        << std::setw(12) << "Max(ms)\n";
    out << std::string(85, '-') << "\n";
    
    for (const auto& pair : mMetrics) {
        const auto& m = pair.second;
        out << std::left << std::setw(25) << m.name 
            << std::setw(10) << m.callCount 
            << std::setw(12) << std::fixed << std::setprecision(2) << m.totalTimeMs
            << std::setw(12) << std::fixed << std::setprecision(3) << m.avgTimeMs
            << std::setw(12) << std::fixed << std::setprecision(3) << m.minTimeMs
            << std::setw(12) << std::fixed << std::setprecision(3) << m.maxTimeMs << "\n";
    }
}

std::string PerformanceProfiler::generateReport() const {
    std::ostringstream oss;
    printReport(oss);
    return oss.str();
}

// DiagnosticLogger 实现
DiagnosticLogger& DiagnosticLogger::getInstance() {
    static DiagnosticLogger instance;
    return instance;
}

DiagnosticLogger::DiagnosticLogger() 
    : mMinLevel(Level::Debug) {
}

DiagnosticLogger::~DiagnosticLogger() {
}

std::string getLevelString(DiagnosticLogger::Level level) {
    switch (level) {
        case DiagnosticLogger::Level::Debug: return "[DEBUG]";
        case DiagnosticLogger::Level::Info: return "[INFO]";
        case DiagnosticLogger::Level::Warning: return "[WARNING]";
        case DiagnosticLogger::Level::Error: return "[ERROR]";
    }
    return "[UNKNOWN]";
}

void DiagnosticLogger::log(Level level, const std::string& message, 
                          const std::string& file, int line) {
    if (level < mMinLevel) return;
    
    std::ostringstream oss;
    oss << getLevelString(level) << " ";
    
    if (!file.empty()) {
        oss << "[" << file;
        if (line > 0) {
            oss << ":" << line;
        }
        oss << "] ";
    }
    
    oss << message;
    mLogs.push_back(oss.str());
}

void DiagnosticLogger::debug(const std::string& message, const std::string& file, int line) {
    log(Level::Debug, message, file, line);
}

void DiagnosticLogger::info(const std::string& message, const std::string& file, int line) {
    log(Level::Info, message, file, line);
}

void DiagnosticLogger::warning(const std::string& message, const std::string& file, int line) {
    log(Level::Warning, message, file, line);
}

void DiagnosticLogger::error(const std::string& message, const std::string& file, int line) {
    log(Level::Error, message, file, line);
}

void DiagnosticLogger::clear() {
    mLogs.clear();
}

void DiagnosticLogger::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    for (const auto& log : mLogs) {
        file << log << "\n";
    }
}

// MemoryTracker 实现
MemoryTracker& MemoryTracker::getInstance() {
    static MemoryTracker instance;
    return instance;
}

MemoryTracker::MemoryTracker() 
    : mTotalAllocatedBytes(0), mAllocationCount(0), mDeallocationCount(0) {
}

MemoryTracker::~MemoryTracker() {
}

void MemoryTracker::trackAllocation(const void* ptr, size_t size, const std::string& type) {
    if (ptr) {
        mAllocations[ptr] = { size, type };
        mTotalAllocatedBytes += size;
        mAllocationCount++;
    }
}

void MemoryTracker::trackDeallocation(const void* ptr) {
    auto it = mAllocations.find(ptr);
    if (it != mAllocations.end()) {
        mTotalAllocatedBytes -= it->second.size;
        mAllocations.erase(it);
        mDeallocationCount++;
    }
}

void MemoryTracker::reset() {
    mAllocations.clear();
    mTotalAllocatedBytes = 0;
    mAllocationCount = 0;
    mDeallocationCount = 0;
}

void MemoryTracker::printReport(std::ostream& out) const {
    out << "========================================\n";
    out << "  Memory Tracker Report\n";
    out << "========================================\n";
    
    out << "Total allocated bytes: " << mTotalAllocatedBytes << "\n";
    out << "Allocation count: " << mAllocationCount << "\n";
    out << "Deallocation count: " << mDeallocationCount << "\n";
    out << "Outstanding allocations: " << mAllocations.size() << "\n";
}

// SystemInfo 实现
std::string SystemInfo::getOSName() {
#ifdef __APPLE__
    return "macOS";
#elif _WIN32
    return "Windows";
#else
    return "Linux";
#endif
}

std::string SystemInfo::getOSVersion() {
    return "Unknown";
}

std::string SystemInfo::getArchitecture() {
    return "x86_64";
}

int SystemInfo::getProcessorCount() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

size_t SystemInfo::getAvailableMemory() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (size_t)status.ullAvailPhys;
#elif defined(__APPLE__)
    int mib[2];
    size_t length;
    int64_t availableMemory = 0;
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(availableMemory);
    if (sysctl(mib, 2, &availableMemory, &length, NULL, 0) == 0) {
        return (size_t)availableMemory;
    }
    return 0;
#else
    size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
    return pageSize * (size_t)sysconf(_SC_AVPHYS_PAGES);
#endif
}

std::string SystemInfo::getCurrentDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string SystemInfo::getWinDarlingVersion() {
    return "0.5.0";
}

void SystemInfo::printAllInfo(std::ostream& out) {
    out << "========================================\n";
    out << "  System Information\n";
    out << "========================================\n";
    out << "OS: " << getOSName() << "\n";
    out << "Version: " << getOSVersion() << "\n";
    out << "Architecture: " << getArchitecture() << "\n";
    out << "Processors: " << getProcessorCount() << "\n";
    out << "Available Memory: " << getAvailableMemory() / (1024 * 1024) << " MB\n";
    out << "Current Date/Time: " << getCurrentDateTime() << "\n";
    out << "WinDarling Version: " << getWinDarlingVersion() << "\n";
}

std::string SystemInfo::generateAllInfo() {
    std::ostringstream oss;
    printAllInfo(oss);
    return oss.str();
}

} // namespace WinDarling
