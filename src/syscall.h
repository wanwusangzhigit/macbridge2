#pragma once

#include <stdint.h>
#include "platform.h"

// 定义 Mach 相关类型
typedef uint32_t mach_port_t;
typedef uint32_t mach_msg_timeout_t;
typedef uint32_t thread_t;
typedef uint32_t task_t;

// BSD 系统调用编号
#define SYS_open        0
#define SYS_close       1
#define SYS_read        3
#define SYS_write       4
#define SYS_lseek       8
#define SYS_stat        18
#define SYS_fstat       28
#define SYS_getpid      20
#define SYS_getppid     21
#define SYS_getuid      24
#define SYS_geteuid     25
#define SYS_getgid      26
#define SYS_getegid     27
#define SYS_brk         12
#define SYS_mmap        47
#define SYS_munmap      48
#define SYS_ioctl       54
#define SYS_access      33
#define SYS_chmod       33
#define SYS_chown       38
#define SYS_rename      40
#define SYS_unlink      45

// Mach 系统调用编号
#define SYS_mach_msg    200
#define SYS_thread_self 201
#define SYS_task_self   202

// 系统调用处理函数
typedef int (*syscall_handler_t)(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// 初始化系统调用表
void init_syscall_table(void);

// 处理系统调用
int handle_syscall(int syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// BSD 系统调用实现
int sys_open(const char* path, int flags, int mode);
int sys_close(int fd);
ssize_t sys_read(int fd, void* buf, size_t count);
ssize_t sys_write(int fd, const void* buf, size_t count);
off_t sys_lseek(int fd, off_t offset, int whence);
int sys_stat(const char* path, struct stat* buf);
int sys_fstat(int fd, struct stat* buf);
pid_t sys_getpid(void);
pid_t sys_getppid(void);
uid_t sys_getuid(void);
uid_t sys_geteuid(void);
gid_t sys_getgid(void);
gid_t sys_getegid(void);
void* sys_brk(void* addr);
void* sys_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int sys_munmap(void* addr, size_t length);
int sys_ioctl(int fd, unsigned long request, void* arg);
int sys_access(const char* path, int mode);
int sys_chmod(const char* path, mode_t mode);
int sys_chown(const char* path, uid_t owner, gid_t group);
int sys_rename(const char* oldpath, const char* newpath);
int sys_unlink(const char* path);

// Mach 系统调用实现
int sys_mach_msg(void* msg, int option, size_t send_size, size_t rcv_size, mach_port_t rcv_name, mach_msg_timeout_t timeout, mach_port_t notify);
thread_t sys_thread_self(void);
task_t sys_task_self(void);
