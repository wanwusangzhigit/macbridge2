# WinDarling - 在 Windows/Linux 上运行 macOS 应用

WinDarling 是一个"反向 Wine"项目，旨在不依赖虚拟机或模拟器，直接在 Windows/Linux 上加载并运行 macOS Mach-O 可执行文件和应用程序。

## 项目特性

- **Mach-O 加载器**：完整解析和加载 macOS 可执行文件格式（32-bit & 64-bit）
- **动态链接器模拟**：模拟 macOS 的 dyld 功能，支持标准 C 库函数
- **系统调用翻译**：将 50+ 个 BSD/macOS 系统调用翻译到宿主平台
- **虚拟文件系统**：支持 macOS 风格的路径和文件系统虚拟化
- **应用包管理**：完整的 .app 包解析、安装、卸载功能
- **持久化存储**：应用状态自动保存和加载
- **详细的统计**：系统调用和性能分析

## 项目结构

```
WinDarling/
├── src/                      # C 核心源代码
│   ├── macho.c/h            # Mach-O 解析器
│   ├── dyld.c/h             # 动态链接器模拟
│   ├── syscall.c/h          # 系统调用翻译
│   ├── vfs.c/h              # 虚拟文件系统
│   ├── platform.c/h         # 平台抽象层
│   ├── app_bundle.c/h       # 应用包管理
│   ├── test_loader.c        # Mach-O 测试工具
│   └── app_loader.c         # 应用加载器
├── bin/                      # 编译后的可执行文件
├── Applications/             # 已安装的应用
├── TestApp.app/              # 示例测试应用
│   └── Contents/
│       ├── Info.plist        # 应用配置
│       └── MacOS/
│           └── TestApp       # Mach-O 可执行文件
├── build.sh                  # Linux/macOS 构建脚本
├── build.bat                 # Windows 构建脚本
├── README.md                 # 项目说明 (本文件)
├── USAGE.md                  # 详细使用指南
└── SYSTEM_CALLS.md           # 系统调用文档
```

## 快速开始

### 1. 构建项目

#### Linux/macOS

```bash
./build.sh
```

#### Windows

```cmd
build.bat
```

### 2. 运行应用加载器

```bash
# 查看帮助
./bin/app_loader help

# 安装测试应用
./bin/app_loader install TestApp.app

# 列出已安装的应用
./bin/app_loader list

# 测试 Mach-O 加载
./bin/app_loader test TestApp.app/Contents/MacOS/TestApp
```

## 主要功能模块

| 模块 | 说明 | 状态 |
|------|------|------|
| Mach-O 解析器 | 解析 32/64-bit 可执行文件 | ✅ 完整 |
| 动态链接器 | 标准库符号，库加载 | ✅ 基础 |
| 系统调用层 | 50+ 个系统调用实现 | ✅ 完整 |
| 虚拟文件系统 | 路径映射，文件操作 | ✅ 完整 |
| 应用包管理 | 安装/卸载/列表 | ✅ 完整 |
| 持久化存储 | 状态保存和加载 | ✅ 完整 |

## 系统调用覆盖

项目实现了 50+ 个 macOS 系统调用的翻译层，包括：
- **文件操作**：open, close, read, write, stat, etc.
- **进程控制**：exit, getpid, getppid, etc.
- **内存管理**：mmap, munmap, brk, mprotect, etc.
- **权限管理**：chmod, chown, access, etc.
- **IPC**：Mach IPC 基础实现
- **网络**：socket API 框架

详见 [SYSTEM_CALLS.md](file:///workspace/SYSTEM_CALLS.md)

## 详细文档

- [USAGE.md](file:///workspace/USAGE.md) - 完整的使用指南
- [SYSTEM_CALLS.md](file:///workspace/SYSTEM_CALLS.md) - 系统调用参考文档

## 技术架构

1. **Mach-O 解析器**：解析 macOS 可执行文件格式，提取段、节和符号信息
2. **动态链接器**：模拟 dyld，提供标准库函数和动态库加载
3. **系统调用翻译**：将 macOS 系统调用转换为宿主平台 API
4. **虚拟文件系统**：处理 macOS 风格路径和文件系统抽象
5. **应用包管理**：完整的应用安装、卸载、启动流程
6. **持久化**：应用状态的保存和恢复

## 开发状态

当前版本：v0.6.0（增强版本）

### 已完成
- ✅ Mach-O 文件解析（32-bit & 64-bit）
- ✅ 段内存映射和重定位基础
- ✅ 应用包管理（安装/卸载/列表）
- ✅ 虚拟文件系统
- ✅ 50+ 系统调用实现
- ✅ 动态链接器基础
- ✅ 完整的文档
- ✅ 持久化存储
- ✅ 系统调用统计

### 下一步
- 📋 完整的 Mach-O 执行引擎
- 📋 Cocoa/Foundation 框架支持
- 📋 更全面的网络支持
- 📋 性能优化和测试

## 示例使用

### 应用安装和管理
```bash
# 安装
./bin/app_loader install TestApp.app

# 列出
./bin/app_loader list

# 卸载
./bin/app_loader uninstall com.example.TestApp
```

### Mach-O 测试
```bash
# 解析和显示 Mach-O 结构
./bin/app_loader test TestApp.app/Contents/MacOS/TestApp
```

## 系统要求

### Linux
- GCC 5+ 或 Clang
- POSIX 系统调用支持
- pthread 支持

### Windows
- MSVC 2017+ 或 MinGW
- Windows 7+
- Win32 API

### macOS
- Xcode Command Line Tools
- macOS 10.11+

## 参考项目

- [Darling](https://github.com/darlinghq/darling) - macOS 应用在 Linux 上运行
- [Wine](https://www.winehq.org/) - Windows 应用在 Linux/macOS 上运行

## 贡献指南

欢迎贡献！请确保：
1. 代码符合项目风格
2. 添加适当的文档
3. 测试通过后再提交
4. 更新相关文档（README/USAGE/SYSTEM_CALLS）

## 许可证

本项目采用 MIT 许可证。
