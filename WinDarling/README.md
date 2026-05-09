# WinDarling - A macOS-compatible framework for non-macOS platforms

WinDarling is a reverse Wine project that provides macOS-like APIs for building applications on non-macOS platforms.

## Project Structure

```
WinDarling/
├── include/
│   ├── foundation/
│   │   └── FoundationCpp.h
│   ├── appkit/
│   │   └── AppKitCpp.h
│   └── utils/
│       ├── diagnostics.h
│       └── exceptions.h
├── src/
│   ├── foundation/
│   │   └── FoundationCpp.cpp
│   ├── appkit/
│   │   └── AppKitCpp.cpp
│   └── utils/
│       └── diagnostics.cpp
├── tests/
│   ├── test_foundation.cpp
│   └── test_appkit.cpp
├── examples/
│   ├── hello_world.cpp
│   ├── advanced_demo.cpp
│   └── diagnostics_demo.cpp
└── CMakeLists.txt
```

## Modules

### 1. Foundation Framework

- **NSString**: String management and manipulation
- **NSArray/NSMutableArray**: Collection management
- **NSDictionary/NSMutableDictionary**: Key-value storage
- **NSNumber/NSData/NSDate**: Data type wrappers
- **NSUserDefaults**: User preferences storage
- **NSNotificationCenter**: Observer pattern for notifications
- **NSTimer**: Timed event execution
- **NSFileManager**: File and directory operations
- **NSLock**: Thread synchronization primitives
- **NSProcessInfo/NSBundle**: Application metadata

### 2. AppKit Framework (UI Components)

- **NSView**: Base view class
- **NSButton**: Interactive button
- **NSTextField**: Text input and label
- **NSTextView**: Rich text editor
- **NSScrollView**: Scrollable container
- **NSSlider**: Value slider control
- **NSProgressIndicator**: Progress visualization
- **NSImageView**: Image display
- **NSWindow**: Window management
- **NSApplication**: Application lifecycle

### 3. Diagnostics & Tools

- **PerformanceTimer**: Time measurements
- **PerformanceProfiler**: Performance profiling and reports
- **DiagnosticLogger**: Structured logging system
- **MemoryTracker**: Memory allocation tracking
- **SystemInfo**: System information collection

## Building the Project

### Prerequisites

- C++17 compatible compiler (GCC 8+, Clang 6+, MSVC 2019+)
- CMake 3.10+

### Compilation

```bash
cd WinDarling
mkdir -p build
cd build
cmake ..
make -j4
```

### Running Tests

```bash
cd build
./test_foundation
./test_appkit
```

### Running Examples

```bash
cd build
./hello_world
./advanced_demo
./diagnostics_demo
```

## Quick Start

### Simple Application Example

```cpp
#include "appkit/AppKitCpp.h"
#include "foundation/FoundationCpp.h"

using namespace WinDarling;

int main() {
    NSWindow* window = new NSWindow();
    window->setTitle("Hello WinDarling!");
    
    NSButton* button = NSButton::buttonWithTitle("Click Me");
    button->setFrame(NSRect(20, 20, 100, 30));
    
    NSView* contentView = new NSView();
    contentView->addSubview(button);
    window->setContentView(contentView);
    
    window->makeKeyAndOrderFront();
    
    delete window;
    return 0;
}
```

### Using Foundation API

```cpp
#include "foundation/FoundationCpp.h"
#include <iostream>

using namespace WinDarling;

int main() {
    // Using UserDefaults
    NSUserDefaults* defaults = NSUserDefaults::standardUserDefaults();
    defaults->setStringForKey("WinDarling", "appName");
    std::cout << defaults->stringForKey("appName") << std::endl;
    
    // Using Notifications
    NSNotificationCenter* center = NSNotificationCenter::defaultCenter();
    center->addObserver("MyEvent", nullptr, [](void*) {
        std::cout << "Notification received!" << std::endl;
    });
    center->postNotificationName("MyEvent");
    
    return 0;
}
```

### Using Diagnostics

```cpp
#include "utils/diagnostics.h"
#include <iostream>

using namespace WinDarling;

int main() {
    // System Info
    SystemInfo::printAllInfo(std::cout);
    
    // Performance Profiling
    PerformanceProfiler& profiler = PerformanceProfiler::getInstance();
    profiler.startTiming("MyTask");
    // ... do work
    profiler.stopTiming("MyTask");
    profiler.printReport(std::cout);
    
    // Memory Tracking
    MemoryTracker& tracker = MemoryTracker::getInstance();
    tracker.printReport(std::cout);
    
    return 0;
}
```

## Version

Current WinDarling Version: 0.5.0

## License

WinDarling is open-source software. The license file can be found in the project root.

## Contributing

Contributions are welcome! Please check out the contributing guide (to be written).
