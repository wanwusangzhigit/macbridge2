
# WinDarling 使用指南

## 项目概述

WinDarling 是一个实验性的项目，旨在提供 macOS 兼容性层，允许在非 macOS 平台（如 Windows、Linux）上加载和运行 macOS 应用程序。

## 项目结构

```
.
├── src/                          # C 核心实现
│   ├── macho.c/h                 # Mach-O 文件格式解析器
│   ├── dyld.c/h                  # 动态链接器模拟器
│   ├── syscall.c/h               # 系统调用转换层
│   ├── vfs.c/h                   # 虚拟文件系统
│   ├── platform.c/h              # 平台抽象层
│   ├── app_bundle.c/h            # .app 应用包管理器（新增）
│   ├── app_loader.c              # 应用加载器主程序（新增）
│   └── test_loader.c             # 简单的 Mach-O 测试加载器
├── WinDarling/                   # C++ 高级框架
│   ├── include/
│   │   ├── foundation/FoundationCpp.h
│   │   ├── appkit/AppKitCpp.h
│   │   └── utils/diagnostics.h
│   └── src/
│       ├── foundation/FoundationCpp.cpp
│       ├── appkit/AppKitCpp.cpp
│       └── utils/diagnostics.cpp
├── TestApp.app/                  # 示例测试应用包（新增）
│   └── Contents/
│       ├── Info.plist
│       ├── MacOS/
│       └── Resources/
├── build.sh                      # Linux/macOS 构建脚本
├── build.bat                     # Windows 构建脚本
├── README.md
└── USAGE.md                      # 本文件
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

构建成功后，可执行文件将位于 `bin/` 目录下：
- `test_loader` - 简单的 Mach-O 测试加载器
- `app_loader` - 功能完整的应用包加载器（新增）

## 使用说明

### app_loader - 应用包加载器（推荐）

`app_loader` 是一个功能更强大的工具，可以加载、安装和管理 macOS 的 .app 应用包。

#### 运行自测试

```bash
# Linux/macOS
bin/app_loader test

# Windows
bin\app_loader.exe test
```

这会测试 Info.plist 解析功能和应用包管理器的基本功能。

#### 加载并运行应用包

```bash
# Linux/macOS
bin/app_loader load &lt;app_bundle_path&gt;

# Windows
bin\app_loader.exe load &lt;app_bundle_path&gt;
```

例如，加载我们提供的测试应用包：
```bash
# Linux/macOS
bin/app_loader load TestApp.app

# Windows
bin\app_loader.exe load TestApp.app
```

#### 安装应用包

```bash
# Linux/macOS
bin/app_loader install &lt;app_bundle_path&gt;

# Windows
bin\app_loader.exe install &lt;app_bundle_path&gt;
```

#### 列出已安装的应用

```bash
# Linux/macOS
bin/app_loader list

# Windows
bin\app_loader.exe list
```

#### 卸载应用包

```bash
# Linux/macOS
bin/app_loader uninstall &lt;bundle_id&gt;

# Windows
bin\app_loader.exe uninstall &lt;bundle_id&gt;
```

#### 加载独立的 Mach-O 可执行文件

```bash
# Linux/macOS
bin/app_loader &lt;macho_file&gt;

# Windows
bin\app_loader.exe &lt;macho_file&gt;
```

### test_loader - 简单测试加载器

`test_loader` 是一个基础的 Mach-O 文件加载器，仅用于测试基本功能：

```bash
# Linux/macOS
bin/test_loader &lt;macho_executable&gt;

# Windows
bin\test_loader.exe &lt;macho_executable&gt;
```

## .app 应用包结构

标准的 macOS .app 应用包结构如下：

```
YourApp.app/
└── Contents/
    ├── Info.plist          # 应用程序配置文件
    ├── MacOS/              # 可执行文件目录
    │   └── YourApp         # 主可执行文件
    └── Resources/          # 资源文件目录
        ├── AppIcon.icns
        └── ...
```

### Info.plist 必需字段

- `CFBundleExecutable` - 可执行文件的名称
- `CFBundleIdentifier` - 唯一的应用程序包标识符（如 `com.example.MyApp`）
- `CFBundleName` - 应用程序的显示名称
- `CFBundleShortVersionString` - 应用程序的版本号

### 创建自己的测试应用包

1. 创建目录结构：
   ```bash
   mkdir -p MyApp.app/Contents/MacOS
   mkdir -p MyApp.app/Contents/Resources
   ```

2. 创建 Info.plist 文件：
   ```xml
   &lt;?xml version="1.0" encoding="UTF-8"?&gt;
   &lt;!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"&gt;
   &lt;plist version="1.0"&gt;
   &lt;dict&gt;
       &lt;key&gt;CFBundleExecutable&lt;/key&gt;&lt;string&gt;MyApp&lt;/string&gt;
       &lt;key&gt;CFBundleIdentifier&lt;/key&gt;&lt;string&gt;com.example.MyApp&lt;/string&gt;
       &lt;key&gt;CFBundleName&lt;/key&gt;&lt;string&gt;My Application&lt;/string&gt;
       &lt;key&gt;CFBundleShortVersionString&lt;/key&gt;&lt;string&gt;1.0.0&lt;/string&gt;
   &lt;/dict&gt;
   &lt;/plist&gt;
   ```

3. 将 Mach-O 可执行文件放入 `MyApp.app/Contents/MacOS/`

4. 使用 app_loader 加载：
   ```bash
   bin/app_loader load MyApp.app
   ```

## 核心功能模块

### 1. Mach-O 解析器 (macho.c)

- 解析 Mach-O 头部（32位和64位）
- 识别段和节
- 加载命令解析
- 支持可执行文件、动态库和插件包

### 2. 动态链接器模拟器 (dyld.c)

- 符号表管理
- 符号查找和解析
- 动态库加载（框架）

### 3. 系统调用翻译层 (syscall.c)

- 将 BSD/macOS 系统调用转换为 Windows 或 Linux API
- 文件操作
- 进程管理
- 内存操作

### 4. 虚拟文件系统 (vfs.c)

- 模拟 macOS 文件系统层次
- 路径转换
- 文件访问模拟

### 5. 应用包管理器 (app_bundle.c)（新增）

- .app 包解析
- Info.plist 读取
- 应用安装/卸载管理
- 应用启动接口

## 当前限制和注意事项

### 已实现功能

- ✅ Mach-O 文件头部解析
- ✅ 段加载和内存映射
- ✅ 基本的 Info.plist 解析
- ✅ 应用包管理（安装/卸载/列表）
- ✅ 简单的动态链接器框架
- ✅ 跨平台构建支持（Windows、Linux、macOS）

### 待实现功能

- ⏳ 完整的系统调用翻译
- ⏳ 动态库加载和链接
- ⏳ Objective-C 运行时支持
- ⏳ AppKit UI 框架
- ⏳ Foundation 框架完善
- ⏳ 调试和性能分析工具

### 注意事项

1. 这是一个实验性项目，不能保证能运行生产级别的 macOS 应用
2. 需要真实的 macOS Mach-O 可执行文件才能测试完整功能
3. 建议先使用简单的命令行工具测试
4. 某些功能可能在特定平台上有限制

## 扩展和贡献

### 添加新的系统调用

编辑 `src/syscall.c` 来添加新的系统调用处理。

### 增强 Mach-O 解析

在 `src/macho.c` 中添加新的加载命令解析。

### 添加平台特定代码

使用预处理器宏来分隔平台特定代码：
- `_WIN32` 用于 Windows
- `__APPLE__` 用于 macOS
- `__linux__` 用于 Linux

## 调试提示

- 使用详细输出来调试加载过程
- 检查内存映射是否正确
- 验证系统调用转换
- 使用 `app_loader test` 运行自测试

## 参考资源

- [Mach-O File Format Reference](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/MachOTopics/01-Introduction/introduction.html)
- [OS X ABI Mach-O File Format Reference](https://github.com/aidansteele/osx-abi-macho-file-format-reference)
- [Darling Project](https://www.darlinghq.org/) - macOS 兼容层（在 Linux 上）
- [Wine Project](https://www.winehq.org/) - Windows 兼容层

## 许可证

本项目采用 MIT 许可证。

## 更新日志

### v0.5.0 (2026-05-02)
- 新增 .app 应用包管理器
- 新增功能完整的 app_loader 工具
- 新增 Info.plist 解析功能
- 增强跨平台兼容性
- 更新构建脚本
- 添加示例测试应用包

### v0.4.0 (2026-05-01)
- 基础 Mach-O 加载器
- 简单的 dyld 模拟器
- 基础系统调用转换
- 虚拟文件系统
- 跨平台构建支持

