#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "utils/diagnostics.h"

using namespace WinDarling;

// 模拟一个耗时函数
void doWork() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  WinDarling Diagnostics Demo\n";
    std::cout << "========================================\n\n";
    
    // 获取系统信息
    std::cout << "1. System Information\n";
    std::cout << "----------------------------------------\n";
    SystemInfo::printAllInfo(std::cout);
    std::cout << "\n";
    
    // 性能分析
    std::cout << "2. Performance Profiling\n";
    std::cout << "----------------------------------------\n";
    
    {
        PerformanceTimer timer("Initialize App");
        timer.start();
        
        // 模拟初始化
        for (int i = 0; i < 3; i++) {
            doWork();
        }
        timer.stop();
        std::cout << "Initialization took: " << timer.getElapsedMilliseconds() << " ms\n";
    }
    
    // 使用分析器
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    profiler.startTiming("First Task");
    doWork();
    profiler.stopTiming("First Task");
    
    profiler.startTiming("Second Task");
    doWork();
    doWork();
    profiler.stopTiming("Second Task");
    
    profiler.startTiming("Third Task");
    for (int i = 0; i < 5; i++) {
        doWork();
    }
    profiler.stopTiming("Third Task");
    
    profiler.printReport(std::cout);
    std::cout << "\n";
    
    // 诊断日志
    std::cout << "3. Diagnostic Logging\n";
    std::cout << "----------------------------------------\n";
    
    DiagnosticLogger& logger = DiagnosticLogger::getInstance();
    logger.debug("Starting application demo");
    logger.info("System initialized successfully");
    logger.warning("Low disk space detected");
    logger.error("Failed to load configuration");
    
    std::cout << "Generated " << logger.getLogs().size() << " log entries\n";
    for (const auto& log : logger.getLogs()) {
        std::cout << log << "\n";
    }
    
    // 内存跟踪
    std::cout << "\n4. Memory Tracking\n";
    std::cout << "----------------------------------------\n";
    
    MemoryTracker& tracker = MemoryTracker::getInstance();
    
    // 模拟一些内存分配
    std::vector<int*> allocations;
    for (int i = 0; i < 5; i++) {
        int* ptr = new int[1024];
        tracker.trackAllocation(ptr, 1024 * sizeof(int), "int[]");
        allocations.push_back(ptr);
    }
    
    // 释放一些内存
    for (int i = 0; i < 3; i++) {
        delete[] allocations[i];
        tracker.trackDeallocation(allocations[i]);
    }
    
    tracker.printReport(std::cout);
    
    // 清理剩下的分配
    for (size_t i = 3; i < allocations.size(); i++) {
        delete[] allocations[i];
        tracker.trackDeallocation(allocations[i]);
    }
    
    std::cout << "\n========================================\n";
    std::cout << "  All diagnostics demo completed\n";
    std::cout << "========================================\n";
    
    return 0;
}
