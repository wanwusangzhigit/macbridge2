#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <chrono>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <iostream>

namespace WinDarling {

// 性能计时器
class PerformanceTimer {
public:
    PerformanceTimer();
    explicit PerformanceTimer(const std::string& name);
    ~PerformanceTimer();
    
    void start();
    void stop();
    void reset();
    
    double getElapsedSeconds() const;
    double getElapsedMilliseconds() const;
    double getElapsedMicroseconds() const;
    
    const std::string& getName() const { return mName; }
    bool isRunning() const { return mIsRunning; }
    
private:
    std::string mName;
    std::chrono::high_resolution_clock::time_point mStart;
    std::chrono::high_resolution_clock::time_point mEnd;
    bool mIsRunning;
};

// 性能统计信息
struct PerformanceMetric {
    std::string name;
    int callCount;
    double totalTimeMs;
    double avgTimeMs;
    double minTimeMs;
    double maxTimeMs;
    
    PerformanceMetric() 
        : callCount(0), totalTimeMs(0.0), avgTimeMs(0.0), 
          minTimeMs(1e30), maxTimeMs(0.0) {}
};

// 性能分析器
class PerformanceProfiler {
public:
    static PerformanceProfiler& getInstance();
    
    void startTiming(const std::string& name);
    void stopTiming(const std::string& name);
    void reset(const std::string& name);
    void resetAll();
    
    const std::map<std::string, PerformanceMetric>& getMetrics() const {
        return mMetrics;
    }
    
    void printReport(std::ostream& out = std::cout) const;
    std::string generateReport() const;
    
private:
    PerformanceProfiler();
    ~PerformanceProfiler();
    
    std::map<std::string, PerformanceTimer> mActiveTimers;
    std::map<std::string, PerformanceMetric> mMetrics;
};

// 诊断记录器
class DiagnosticLogger {
public:
    enum Level { Debug, Info, Warning, Error };
    
    static DiagnosticLogger& getInstance();
    
    void log(Level level, const std::string& message, 
             const std::string& file = "", int line = 0);
    void debug(const std::string& message, const std::string& file = "", int line = 0);
    void info(const std::string& message, const std::string& file = "", int line = 0);
    void warning(const std::string& message, const std::string& file = "", int line = 0);
    void error(const std::string& message, const std::string& file = "", int line = 0);
    
    void clear();
    void saveToFile(const std::string& filename);
    
    const std::vector<std::string>& getLogs() const { return mLogs; }
    
private:
    DiagnosticLogger();
    ~DiagnosticLogger();
    
    std::vector<std::string> mLogs;
    Level mMinLevel;
};

// 内存跟踪器
class MemoryTracker {
public:
    static MemoryTracker& getInstance();
    
    void trackAllocation(const void* ptr, size_t size, const std::string& type = "");
    void trackDeallocation(const void* ptr);
    
    size_t getAllocatedBytes() const { return mTotalAllocatedBytes; }
    size_t getAllocationCount() const { return mAllocationCount; }
    size_t getDeallocationCount() const { return mDeallocationCount; }
    
    void reset();
    void printReport(std::ostream& out = std::cout) const;
    
private:
    MemoryTracker();
    ~MemoryTracker();
    
    struct AllocationInfo {
        size_t size;
        std::string type;
    };
    
    std::map<const void*, AllocationInfo> mAllocations;
    size_t mTotalAllocatedBytes;
    size_t mAllocationCount;
    size_t mDeallocationCount;
};

// 系统信息收集器
class SystemInfo {
public:
    static std::string getOSName();
    static std::string getOSVersion();
    static std::string getArchitecture();
    static int getProcessorCount();
    static size_t getAvailableMemory();
    static std::string getCurrentDateTime();
    static std::string getWinDarlingVersion();
    
    static void printAllInfo(std::ostream& out = std::cout);
    static std::string generateAllInfo();
};

} // namespace WinDarling

#endif // DIAGNOSTICS_H
