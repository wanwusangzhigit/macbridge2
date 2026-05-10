# WinDarling 使用指南

## 简介

WinDarling 是一个开源项目，让你能够在非 macOS 平台（Linux、Windows）上运行 macOS Mach-O 可执行文件和应用程序。

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

### 2. 使用 app_loader

#### 查看帮助
```bash
./bin/app_loader help
```

#### 测试 Mach-O 文件
```bash
./bin/app_loader test TestApp.app/Contents/MacOS/TestApp
```

#### 加载应用程序
```bash
./bin/app_loader load TestApp.app
```

#### 安装应用程序
```bash
./bin/app_loader install TestApp.app
```

#### 列出已安装的应用
```bash
./bin/app_loader list
```

#### 卸载应用程序
```bash
./bin/app_loader uninstall com.example.TestApp
```

## 应用包结构

### 标准 macOS 应用包
```
MyApp.app/
├── Contents/
│   ├── Info.plist          # 应用配置信息
│   ├── MacOS/
│   │   └── MyApp           # Mach-O 可执行文件
│   ├── Resources/          # 资源文件 (可选)
│   │   └── ...
│   └── Frameworks/         # 内嵌框架 (可选)
│       └── ...
```

### Info.plist 必需字段
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>TestApp</string>
    <key>CFBundleIdentifier</key>
    <string>com.example.TestApp</string>
    <key>CFBundleName</key>
    <string>Test App</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
</dict>
</plist>
```

## 架构说明

### 核心模块

#### 1. Mach-O 解析器 ([macho.h](file:///workspace/src/macho.h)/[macho.c](file:///workspace/src/macho.c))
- 解析 Mach-O 文件头
- 加载命令处理
- 段和节管理
- 支持 32-bit 和 64-bit 格式

#### 2. 动态链接器 ([dyld.h](file:///workspace/src/dyld.h)/[dyld.c](file:///workspace/src/dyld.c))
- 加载动态库
- 符号查找和解析
- 模拟标准 C 库函数
- 延迟绑定处理

#### 3. 系统调用翻译层 ([syscall.h](file:///workspace/src/syscall.h)/[syscall.c](file:///workspace/src/syscall.c))
- 翻译 macOS 系统调用到宿主系统
- 文件操作、内存管理、进程控制
- Mach IPC 模拟
- 详细的调用统计

#### 4. 虚拟文件系统 ([vfs.h](file:///workspace/src/vfs.h)/[vfs.c](file:///workspace/src/vfs.c))
- 路径映射
- 文件系统虚拟化
- 支持 macOS 特殊路径

#### 5. 应用包管理 ([app_bundle.h](file:///workspace/src/app_bundle.h)/[app_bundle.c](file:///workspace/src/app_bundle.c))
- 解析应用包结构
- 应用安装/卸载
- 持久化状态保存
- 应用启动管理

## 命令参考

### app_loader 命令
| 命令 | 参数 | 功能描述 |
|------|------|----------|
| `test` | `<macho-file>` | 测试 Mach-O 加载 |
| `load` | `<app-path>` | 加载并运行应用 |
| `install` | `<app-path>` | 安装应用程序 |
| `list` | (无) | 列出已安装的应用 |
| `uninstall` | `<bundle-id>` | 卸载应用程序 |
| `help` | (无) | 显示帮助信息 |

### test_loader 命令
| 命令 | 参数 | 功能描述 |
|------|------|----------|
| (无) | `<macho-file>` | 解析并显示 Mach-O 结构 |

## 持久化存储

### 应用状态文件
应用列表自动保存到：
```
Applications/apps.dat
```

### 状态自动保存
- ✅ 应用安装后自动保存
- ✅ 应用卸载后自动保存
- ✅ 启动时自动加载已保存的状态
- ✅ 二进制格式存储，加载快速

## 测试示例

### 运行包含的测试应用
```bash
# 安装测试应用
./bin/app_loader install TestApp.app

# 查看已安装的应用
./bin/app_loader list

# 加载和测试 Mach-O
./bin/app_loader test TestApp.app/Contents/MacOS/TestApp

# 卸载
./bin/app_loader uninstall com.example.TestApp
```

## 开发指南

### 添加新的系统调用
1. 在 [syscall.h](file:///workspace/src/syscall.h) 中添加编号定义
2. 在 [syscall.c](file:///workspace/src/syscall.c) 中实现函数
3. 在 `init_syscall_table()` 中注册处理函数
4. 更新 `get_syscall_name()` 和 [SYSTEM_CALLS.md](file:///workspace/SYSTEM_CALLS.md)

### 添加新的动态链接符号
1. 在 `dyld_init()` 中调用 `dyld_add_symbol()`
2. 提供本地实现函数
3. 确保函数签名与 macOS 版本兼容

### 调试技巧
```c
// 启用系统调用统计
print_syscall_stats();

// 查看已加载的模块
// (通过调试器检查 dyld_images)
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

## 常见问题

### Q: Mach-O 文件加载失败
A: 请确认文件格式正确（32-bit 或 64-bit），并且文件未损坏。

### Q: 找不到某些系统调用
A: 检查 [SYSTEM_CALLS.md](file:///workspace/SYSTEM_CALLS.md) 文档，确认该调用是否已实现。

### Q: 如何运行真实的 macOS 应用
A: 当前版本支持加载 Mach-O 和解析应用包，但完整执行还需要框架支持。请查看项目路线图。

## 项目路线图

### 已完成
- ✅ Mach-O 文件解析
- ✅ 应用包管理
- ✅ 虚拟文件系统
- ✅ 动态链接器基础
- ✅ 50+ 系统调用实现
- ✅ 持久化存储
- ✅ 完整的文档

### 下一步
- 📋 真实执行引擎
- 📋 系统框架加载
- 📋 Cocoa 框架支持
- 📋 更好的网络支持
- 📋 性能优化
- 📋 更全面的测试

## 贡献指南

欢迎贡献！请确保：
- 代码符合项目风格
- 添加适当的文档
- 测试通过后再提交
- 更新相关文档

## 许可证

WinDarling 采用 MIT 许可证。详见项目根目录的 LICENSE 文件。
