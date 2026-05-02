# WinDarling 系统调用参考文档

## 概述
WinDarling 实现了 macOS/BSD 系统调用的翻译层，允许 Mach-O 可执行文件在非 macOS 平台上运行。

## 系统调用编号 (macOS x86_64)

### 进程控制
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 1 | SYS_exit | 进程退出 | ✅ 实现 |
| 20 | SYS_getpid | 获取进程 ID | ✅ 实现 |
| 39 | SYS_getppid | 获取父进程 ID | ✅ 实现 |
| 24 | SYS_getuid | 获取用户 ID | ✅ 实现 |
| 25 | SYS_geteuid | 获取有效用户 ID | ✅ 实现 |
| 47 | SYS_getgid | 获取组 ID | ✅ 实现 |
| 43 | SYS_getegid | 获取有效组 ID | ✅ 实现 |

### 文件操作
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 5 | SYS_open | 打开文件 | ✅ 实现 |
| 6 | SYS_close | 关闭文件 | ✅ 实现 |
| 3 | SYS_read | 读取文件 | ✅ 实现 |
| 4 | SYS_write | 写入文件 | ✅ 实现 |
| 199 | SYS_lseek | 文件定位 | ✅ 实现 |
| 188 | SYS_stat | 获取文件状态 | ✅ 实现 |
| 189 | SYS_fstat | 获取 FD 状态 | ✅ 实现 |
| 10 | SYS_unlink | 删除文件 | ✅ 实现 |
| 128 | SYS_rename | 重命名文件 | ✅ 实现 |
| 136 | SYS_mkdir | 创建目录 | ✅ 实现 |
| 137 | SYS_rmdir | 删除目录 | ✅ 实现 |

### 权限管理
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 33 | SYS_access | 检查访问权限 | ✅ 实现 |
| 15 | SYS_chmod | 修改权限 | ✅ 实现 |
| 16 | SYS_chown | 修改所有者 | ✅ 实现 |
| 94 | SYS_fchmod | 修改 FD 权限 | ✅ 简化 |
| 95 | SYS_fchown | 修改 FD 所有者 | ✅ 简化 |

### 内存管理
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 214 | SYS_brk | 设置数据段 | ✅ 实现 |
| 197 | SYS_mmap | 内存映射 | ✅ 实现 |
| 198 | SYS_munmap | 解除映射 | ✅ 实现 |
| 226 | SYS_mprotect | 修改保护 | ✅ 简化 |
| 227 | SYS_msync | 同步内存 | ✅ 简化 |

### 网络操作
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| (TBD) | SYS_socket | 创建套接字 | 📋 框架 |
| (TBD) | SYS_connect | 连接套接字 | 📋 框架 |
| (TBD) | SYS_accept | 接受连接 | 📋 框架 |
| (TBD) | SYS_bind | 绑定地址 | 📋 框架 |
| (TBD) | SYS_listen | 监听连接 | 📋 框架 |

### 文件描述符
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 90 | SYS_dup | 复制 FD | ✅ 实现 |
| 23 | SYS_dup2 | 复制 FD 2 | ✅ 实现 |
| 54 | SYS_ioctl | I/O 控制 | ✅ 简化 |

### Mach IPC
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 200 | SYS_mach_msg | Mach 消息 | ✅ 简化 |
| 201 | SYS_thread_self | 当前线程 | ✅ 实现 |
| 202 | SYS_task_self | 当前任务 | ✅ 实现 |

### 时间相关
| 编号 | 名称 | 说明 | 状态 |
|------|------|------|------|
| 116 | SYS_gettimeofday | 获取时间 | ✅ 简化 |
| 137 | SYS_settimeofday | 设置时间 | ✅ 简化 |

## 实现细节

### 文件描述符映射
系统使用 `fd_map[]` 数组维护内部描述符到外部句柄的映射：
```c
static platform_handle fd_map[1024];  // 最多支持 1024 个文件描述符
static int next_fd = 3;                // 0,1,2 预留给 stdin/stdout/stderr
```

### 系统调用统计
`syscall_stats` 结构记录每次系统调用：
```c
struct {
    int total_calls;                    // 总调用次数
    int syscall_counts[256];            // 每个调用的次数
} syscall_stats;
```

### 辅助函数
```c
// 获取系统调用名称
const char* get_syscall_name(int syscall_num);

// 打印统计信息
void print_syscall_stats(void);
```

## 使用示例

### 初始化系统调用表
```c
init_syscall_table();
```

### 处理单个系统调用
```c
// 打开文件 (模拟 syscall)
int fd = handle_syscall(SYS_open,
                        (uint64_t)path,
                        O_RDONLY, 0, 0, 0, 0);

// 读取文件
ssize_t bytes = handle_syscall(SYS_read,
                                (uint64_t)fd,
                                (uint64_t)buffer,
                                count, 0, 0, 0);
```

### 统计信息
```c
// 打印系统调用使用统计
print_syscall_stats();
```

## 跨平台兼容性

### Windows
在 Windows 上，系统调用被翻译为 Win32 API：
- `open()` → `CreateFile()`
- `read()` → `ReadFile()`
- `write()` → `WriteFile()`

### POSIX
在 Linux/macOS 上，直接映射到本地系统调用。

## 扩展系统调用

要添加新的系统调用：

1. 在 [syscall.h](file:///workspace/src/syscall.h) 中添加编号定义
2. 在 [syscall.c](file:///workspace/src/syscall.c) 中实现函数
3. 在 `init_syscall_table()` 中注册处理函数
4. 更新 `get_syscall_name()` 以支持新名称

## 状态图例
- ✅ **完整实现**：功能完整可用
- 📋 **框架实现**：基本结构存在，但需要进一步开发
- 📝 **待实现**：尚未开发
