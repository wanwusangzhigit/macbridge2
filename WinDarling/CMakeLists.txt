cmake_minimum_required(VERSION 3.10)
project(WinDarling VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Foundation 测试
add_executable(test_foundation
    tests/test_foundation.cpp
    src/foundation/FoundationCpp.cpp
)

# 高级示例
add_executable(advanced_demo
    examples/advanced_demo.cpp
    src/foundation/FoundationCpp.cpp
    src/appkit/AppKitCpp.cpp
)

# 诊断示例
add_executable(diagnostics_demo
    examples/diagnostics_demo.cpp
    src/utils/diagnostics.cpp
)
