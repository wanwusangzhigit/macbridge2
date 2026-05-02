#pragma once

#include <stdint.h>
#include "platform.h"

// 定义 Mach 相关类型
typedef uint32_t mach_port_t;
typedef uint32_t mach_msg_timeout_t;
typedef uint32_t thread_t;
typedef uint32_t task_t;

// BSD 系统调用编号 (macOS/x86_64 - 常用子集)
#define SYS_exit         1
#define SYS_fork         2
#define SYS_read         3
#define SYS_write        4
#define SYS_open         5
#define SYS_close        6
#define SYS_wait4        7
#define SYS_getpid       20
#define SYS_getppid      39
#define SYS_getuid       24
#define SYS_geteuid      25
#define SYS_getgid       47
#define SYS_getegid      43
#define SYS_access       33
#define SYS_chmod        15
#define SYS_chown        16
#define SYS_fchmod       94
#define SYS_fchown       95
#define SYS_rename       128
#define SYS_unlink       10
#define SYS_mkdir        136
#define SYS_rmdir        137
#define SYS_lseek        199
#define SYS_stat         188
#define SYS_fstat        189
#define SYS_utimes       138
#define SYS_brk          214
#define SYS_mmap         197
#define SYS_munmap       198
#define SYS_mprotect     226
#define SYS_msync        227
#define SYS_ioctl        54
#define SYS_dup          90
#define SYS_dup2         23
#define SYS_gettimeofday 116
#define SYS_settimeofday 137
#define SYS_getrlimit    76
#define SYS_setrlimit    75

// Mach 系统调用编号
#define SYS_mach_msg           200
#define SYS_mach_timebase_info 200
#define SYS_thread_self        201
#define SYS_task_self          202
#define SYS_host_self          203

// 套接字地址结构 (简化)
struct sockaddr {
    uint16_t sa_family;
    char     sa_data[14];
};

typedef uint32_t socklen_t;

// 系统调用处理函数
typedef int (*syscall_handler_t)(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// 初始化系统调用表
void init_syscall_table(void);

// 处理系统调用
int handle_syscall(int syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// 进程控制
int sys_exit(int status);
int sys_getpid(void);
int sys_getppid(void);
int sys_getuid(void);
int sys_geteuid(void);
int sys_getgid(void);
int sys_getegid(void);

// 文件操作
int sys_open(const char* path, int flags, int mode);
int sys_close(int fd);
ssize_t sys_read(int fd, void* buf, size_t count);
ssize_t sys_write(int fd, const void* buf, size_t count);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_stat(const char* path, struct stat* buf);
int sys_fstat(int fd, struct stat* buf);
int sys_access(const char* path, int mode);
int sys_chmod(const char* path, mode_t mode);
int sys_chown(const char* path, uid_t owner, gid_t group);
int sys_fchmod(int fd, mode_t mode);
int sys_fchown(int fd, uid_t owner, gid_t group);
int sys_unlink(const char* path);
int sys_rename(const char* oldpath, const char* newpath);
int sys_mkdir(const char* path, mode_t mode);
int sys_rmdir(const char* path);

// 内存管理
void* sys_brk(void* addr);
void* sys_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int sys_munmap(void* addr, size_t length);
int sys_mprotect(void* addr, size_t len, int prot);
int sys_msync(void* addr, size_t len, int flags);

// I/O 控制
int sys_ioctl(int fd, unsigned long request, void* arg);

// 网络操作
int sys_socket(int domain, int type, int protocol);
int sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int sys_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int sys_listen(int sockfd, int backlog);
int sys_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
int sys_getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
int sys_shutdown(int sockfd, int how);

// 文件描述符操作
int sys_dup(int oldfd);
int sys_dup2(int oldfd, int newfd);

// 时间相关
int sys_gettimeofday(void* tv, void* tz);
int sys_settimeofday(void* tv, void* tz);

// Mach 系统调用
int sys_mach_msg(void* msg, int option, size_t send_size, size_t rcv_size, mach_port_t rcv_name, mach_msg_timeout_t timeout, mach_port_t notify);
thread_t sys_thread_self(void);
task_t sys_task_self(void);

// 辅助函数
const char* get_syscall_name(int syscall_num);
void print_syscall_stats(void);
