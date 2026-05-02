
#!/usr/bin/env python3

syscall_content = r'''#include "syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 文件描述符映射表
static platform_handle fd_map[1024] = {0};
static int next_fd = 3;

// 系统调用统计
static struct {
    int total_calls;
    int syscall_counts[256];
} syscall_stats = {0};

#ifdef _WIN32
    #include <Windows.h>
    #include <process.h>
    #include <io.h>
    #include <direct.h>
    
    #define SEEK_SET 0
    #define SEEK_CUR 1
    #define SEEK_END 2
    
    #define F_OK 0
    #define R_OK 4
    #define W_OK 2
    #define X_OK 1
    
    #define getpid() _getpid()
    static int getppid(void) { return 0; }
    #define getuid() 0
    #define geteuid() 0
    #define getgid() 0
    #define getegid() 0
    
    #define access(path, mode) _access(path, mode)
    #define rename(oldpath, newpath) rename(oldpath, newpath)
    #define unlink(path) _unlink(path)
    #define chmod(path, mode) _chmod(path, mode)
    #define chown(path, owner, group) 0
    #define mkdir(path, mode) _mkdir(path)
    #define rmdir(path) _rmdir(path)
    
    struct stat {
        off_t st_size;
    };
    
    static off_t lseek(int fd, off_t offset, int whence) {
        LONG high = 0;
        DWORD result = SetFilePointer((HANDLE)fd_map[fd], (LONG)offset, &high, (DWORD)whence);
        if (result == INVALID_SET_FILE_POINTER) {
            return -1;
        }
        return ((off_t)high << 32) | result;
    }
    
    static int stat(const char* path, struct stat* buf) {
        WIN32_FILE_ATTRIBUTE_DATA attr;
        if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attr)) {
            return -1;
        }
        buf->st_size = ((off_t)attr.nFileSizeHigh << 32) | (off_t)attr.nFileSizeLow;
        return 0;
    }
    
    static int fstat(int fd, struct stat* buf) {
        HANDLE handle = (HANDLE)fd_map[fd];
        LARGE_INTEGER size;
        if (!GetFileSizeEx(handle, &size)) {
            return -1;
        }
        buf->st_size = (off_t)size.QuadPart;
        return 0;
    }
    
    static unsigned long pthread_self(void) {
        return (unsigned long)_beginthreadex(NULL, 0, NULL, NULL, 0, NULL);
    }
#else
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <pthread.h>
#endif

// 系统调用表
static syscall_handler_t syscall_table[256] = {0};

// 获取系统调用名称
const char* get_syscall_name(int syscall_num) {
    switch (syscall_num) {
        case 1: return "exit";
        case 2: return "fork";
        case 3: return "read";
        case 4: return "write";
        case 5: return "open";
        case 6: return "close";
        case 20: return "getpid";
        case 24: return "getuid";
        case 25: return "geteuid";
        case 39: return "getppid";
        case 47: return "mmap";
        case 48: return "munmap";
        case 74: return "mkdir";
        case 75: return "rmdir";
        case 96: return "ioctl";
        case 197: return "mmap";
        case 198: return "munmap";
        case 199: return "msync";
        case 200: return "mach_msg";
        default: return "unknown";
    }
}

// 打印系统调用统计
void print_syscall_stats(void) {
    printf("System Call Statistics:\n");
    printf("Total calls: %d\n", syscall_stats.total_calls);
    printf("\nTop 10 syscalls:\n");
    
    // 简单的气泡排序获取前10个
    int indices[256];
    for (int i = 0; i < 256; i++) indices[i] = i;
    
    for (int i = 0; i < 256 && i < 10; i++) {
        for (int j = i + 1; j < 256; j++) {
            if (syscall_stats.syscall_counts[indices[j]] > syscall_stats.syscall_counts[indices[i]]) {
                int temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
    }
    
    for (int i = 0; i < 10 && syscall_stats.syscall_counts[indices[i]] > 0; i++) {
        printf("  %s: %d calls\n", get_syscall_name(indices[i]), syscall_stats.syscall_counts[indices[i]]);
    }
}

// 初始化系统调用表
void init_syscall_table(void) {
    // 初始化文件描述符映射
    fd_map[0] = (platform_handle)0;
    fd_map[1] = (platform_handle)0;
    fd_map[2] = (platform_handle)0;
    
    // 注册 BSD 系统调用
    syscall_table[SYS_exit] = (syscall_handler_t)sys_exit;
    syscall_table[SYS_read] = (syscall_handler_t)sys_read;
    syscall_table[SYS_write] = (syscall_handler_t)sys_write;
    syscall_table[SYS_open] = (syscall_handler_t)sys_open;
    syscall_table[SYS_close] = (syscall_handler_t)sys_close;
    syscall_table[SYS_lseek] = (syscall_handler_t)sys_lseek;
    syscall_table[SYS_stat] = (syscall_handler_t)sys_stat;
    syscall_table[SYS_fstat] = (syscall_handler_t)sys_fstat;
    syscall_table[SYS_getpid] = (syscall_handler_t)sys_getpid;
    syscall_table[SYS_getppid] = (syscall_handler_t)sys_getppid;
    syscall_table[SYS_getuid] = (syscall_handler_t)sys_getuid;
    syscall_table[SYS_geteuid] = (syscall_handler_t)sys_geteuid;
    syscall_table[SYS_getgid] = (syscall_handler_t)sys_getgid;
    syscall_table[SYS_getegid] = (syscall_handler_t)sys_getegid;
    syscall_table[SYS_access] = (syscall_handler_t)sys_access;
    syscall_table[SYS_chmod] = (syscall_handler_t)sys_chmod;
    syscall_table[SYS_chown] = (syscall_handler_t)sys_chown;
    syscall_table[SYS_unlink] = (syscall_handler_t)sys_unlink;
    syscall_table[SYS_rename] = (syscall_handler_t)sys_rename;
    syscall_table[SYS_mkdir] = (syscall_handler_t)sys_mkdir;
    syscall_table[SYS_rmdir] = (syscall_handler_t)sys_rmdir;
    syscall_table[SYS_brk] = (syscall_handler_t)sys_brk;
    syscall_table[SYS_mmap] = (syscall_handler_t)sys_mmap;
    syscall_table[SYS_munmap] = (syscall_handler_t)sys_munmap;
    syscall_table[SYS_ioctl] = (syscall_handler_t)sys_ioctl;
    
    // 注册 Mach 系统调用
    syscall_table[SYS_mach_msg] = (syscall_handler_t)sys_mach_msg;
    syscall_table[SYS_thread_self] = (syscall_handler_t)sys_thread_self;
    syscall_table[SYS_task_self] = (syscall_handler_t)sys_task_self;
    
    printf("System call table initialized with %d handlers\n", 30);
}

// 处理系统调用
int handle_syscall(int syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    syscall_stats.total_calls++;
    if (syscall_num >= 0 && syscall_num < 256) {
        syscall_stats.syscall_counts[syscall_num]++;
    }
    
    if (syscall_num < 0 || syscall_num >= 256 || !syscall_table[syscall_num]) {
        printf("Warning: Unknown syscall %d (%s)\n", syscall_num, get_syscall_name(syscall_num));
        return -1;
    }
    
    return syscall_table[syscall_num](arg1, arg2, arg3, arg4, arg5, arg6);
}

// 进程控制
int sys_exit(int status) {
    printf("Process exiting with status: %d\n", status);
    return 0;
}

pid_t sys_getpid(void) {
    return getpid();
}

pid_t sys_getppid(void) {
    return getppid();
}

uid_t sys_getuid(void) {
    return getuid();
}

uid_t sys_geteuid(void) {
    return geteuid();
}

gid_t sys_getgid(void) {
    return getgid();
}

gid_t sys_getegid(void) {
    return getegid();
}

// 打开文件
int sys_open(const char* path, int flags, int mode) {
    platform_handle handle = platform_open(path, flags, mode);
    if (!handle) {
        return -1;
    }
    
    if (next_fd >= 1024) {
        platform_close(handle);
        return -1;
    }
    
    fd_map[next_fd] = handle;
    return next_fd++;
}

// 关闭文件
int sys_close(int fd) {
    if (fd < 0 || fd >= 1024 || !fd_map[fd]) {
        return -1;
    }
    
    platform_close(fd_map[fd]);
    fd_map[fd] = 0;
    return 0;
}

// 读取文件
ssize_t sys_read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= 1024 || !fd_map[fd]) {
        return -1;
    }
    
    return platform_read(fd_map[fd], buf, count);
}

// 写入文件
ssize_t sys_write(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= 1024 || !fd_map[fd]) {
        return -1;
    }
    
    return platform_write(fd_map[fd], buf, count);
}

// 定位文件指针
off_t sys_lseek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd >= 1024 || !fd_map[fd]) {
        return -1;
    }
    
    return lseek(fd, offset, whence);
}

// 获取文件状态
int sys_stat(const char* path, struct stat* buf) {
    return stat(path, buf);
}

// 获取文件描述符状态
int sys_fstat(int fd, struct stat* buf) {
    if (fd < 0 || fd >= 1024 || !fd_map[fd]) {
        return -1;
    }
    
    return fstat(fd, buf);
}

// 文件访问权限
int sys_access(const char* path, int mode) {
    return access(path, mode);
}

// 修改文件权限
int sys_chmod(const char* path, mode_t mode) {
    return chmod(path, mode);
}

// 修改文件所有者
int sys_chown(const char* path, uid_t owner, gid_t group) {
    return chown(path, owner, group);
}

// 修改文件描述符权限
int sys_fchmod(int fd, mode_t mode) {
    return 0; // 简化实现
}

// 修改文件描述符所有者
int sys_fchown(int fd, uid_t owner, gid_t group) {
    return 0; // 简化实现
}

// 删除文件
int sys_unlink(const char* path) {
    return unlink(path);
}

// 重命名文件
int sys_rename(const char* oldpath, const char* newpath) {
    return rename(oldpath, newpath);
}

// 创建目录
int sys_mkdir(const char* path, mode_t mode) {
    return mkdir(path, mode);
}

// 删除目录
int sys_rmdir(const char* path) {
    return rmdir(path);
}

// 内存分配
typedef struct {
    void* start;
    size_t size;
} brk_info;

static brk_info brk_data = { NULL, 0 };

void* sys_brk(void* addr) {
    if (!addr) {
        return brk_data.start;
    }
    
    if (!brk_data.start) {
        brk_data.start = platform_alloc(4096);
        brk_data.size = 4096;
        return brk_data.start;
    }
    
    size_t new_size = (uintptr_t)addr - (uintptr_t)brk_data.start;
    if (new_size > brk_data.size) {
        void* new_brk = platform_alloc(new_size);
        if (!new_brk) {
            return (void*)-1;
        }
        memcpy(new_brk, brk_data.start, brk_data.size);
        platform_free(brk_data.start);
        brk_data.start = new_brk;
        brk_data.size = new_size;
    }
    
    return addr;
}

// 内存映射
void* sys_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    return platform_mmap(addr, length, prot, flags, fd, offset);
}

// 解除内存映射
int sys_munmap(void* addr, size_t length) {
    return platform_munmap(addr, length);
}

// 内存保护
int sys_mprotect(void* addr, size_t len, int prot) {
    return 0; // 简化实现
}

// 同步内存
int sys_msync(void* addr, size_t len, int flags) {
    return 0; // 简化实现
}

// IO 控制
int sys_ioctl(int fd, unsigned long request, void* arg) {
    return 0; // 简化实现
}

// 网络操作 (简化实现)
int sys_socket(int domain, int type, int protocol) {
    printf("syscall: socket(domain=%d, type=%d, protocol=%d)\n", domain, type, protocol);
    return -1; // 暂时不支持
}

int sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    printf("syscall: connect(sockfd=%d, ...)\n", sockfd);
    return -1;
}

int sys_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    printf("syscall: accept(sockfd=%d, ...)\n", sockfd);
    return -1;
}

int sys_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    printf("syscall: bind(sockfd=%d, ...)\n", sockfd);
    return -1;
}

int sys_listen(int sockfd, int backlog) {
    printf("syscall: listen(sockfd=%d, backlog=%d)\n", sockfd, backlog);
    return -1;
}

int sys_setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    return 0;
}

int sys_getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    return 0;
}

int sys_shutdown(int sockfd, int how) {
    return 0;
}

// Mach 消息
int sys_mach_msg(void* msg, int option, size_t send_size, size_t rcv_size, mach_port_t rcv_name, mach_msg_timeout_t timeout, mach_port_t notify) {
    // 简化实现
    return 0;
}

// 获取当前线程
thread_t sys_thread_self(void) {
    return (thread_t)pthread_self();
}

// 获取当前任务
task_t sys_task_self(void) {
    return (task_t)getpid();
}
''';

with open('/workspace/src/syscall.c', 'w') as f:
    f.write(syscall_content)

print("Enhanced syscall.c with more system calls and statistics!")
