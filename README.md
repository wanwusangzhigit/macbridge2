# WinDarling - 在 Windows 上运行 macOS 应用

WinDarling 是一个"反向 Wine"项目，旨在不依赖虚拟机或模拟器，直接在 Windows 上加载并运行 macOS 应用。

## 项目特性

- **Mach-O 加载器**：解析和加载 macOS 可执行文件格式
- **动态链接器模拟**：模拟 macOS 的 dyld 功能
- **系统调用翻译**：将 BSD/macOS 系统调用翻译为 Windows API
- **Foundation 框架**：提供 NSString、NSArray、NSDictionary 等核心类
- **AppKit 框架**：提供 NSWindow、NSButton、NSTextField 等 UI 组件

## 项目结构

```
WinDarling/
├── src/                    # C 核心源代码
│   ├── macho.c/h          # Mach-O 解析器
│   ├── dyld.c/h           # 动态链接器模拟
│   ├── syscall.c/h        # 系统调用翻译
│   ├── vfs.c/h            # 虚拟文件系统
│   └── platform.c/h       # 平台抽象层
├── WinDarling/            # C++ 框架实现
│   ├── include/           # 头文件
│   │   ├── foundation/    # Foundation 框架头文件
│   │   ├── appkit/        # AppKit 框架头文件
│   │   └── utils/         # 工具类头文件
│   ├── src/               # 源文件
│   ├── examples/          # 示例代码
│   └── tests/             # 测试代码
├── build.sh               # Linux/macOS 构建脚本
├── build.bat              # Windows 构建脚本
└── CMakeLists.txt         # CMake 配置
```

## 构建项目

### Linux/macOS

```bash
chmod +x build.sh
./build.sh
```

### Windows

```cmd
build.bat
```

### 使用 CMake 构建 C++ 框架

```bash
cd WinDarling
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 运行测试

```bash
# C 核心测试
./bin/test_loader <macho_executable>

# C++ 框架测试
cd WinDarling/build
./test_foundation
./advanced_demo
./diagnostics_demo
```

## 技术架构

1. **Mach-O 解析器**：解析 macOS 可执行文件格式，提取段、节和符号信息
2. **内存映射**：将 Mach-O 段映射到 Windows 进程虚拟地址空间
3. **动态链接**：模拟 dyld 行为，处理动态库加载和符号绑定
4. **系统调用翻译**：将 BSD 系统调用转换为 Windows API 调用

## 开发状态

当前版本：v0.5.0（原型阶段）

- [x] Mach-O 头部解析
- [x] 段映射到内存
- [x] main 函数查找和执行
- [ ] 动态库加载
- [ ] 符号绑定
- [ ] 系统调用翻译
- [ ] Foundation 框架完善
- [ ] GUI 支持

## 参考项目

- [Darling](https://github.com/darlinghq/darling) - macOS 应用在 Linux 上运行
- [Wine](https://www.winehq.org/) - Windows 应用在 Linux/macOS 上运行

## 许可证

本项目采用 MIT 许可证。
