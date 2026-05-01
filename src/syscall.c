#include "syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <Windows.h>
    #include <process.h>
    
    #define pid_t int
    #define uid_t int
    #define gid_t int
    #define mode_t int
    
    #define SEEK_SET 0
    #define SEEK_CUR 1
    #define SEEK_END 2
    
    #define F_OK 0
    #define R_OK 4
    #define W_OK 2
    #define X_OK 1
    
    #define getpid() _getpid()
    #define getppid() _getppid()
    #define getuid() 0
    #define geteuid() 0
    #define getgid() 0
    #define getegid() 0
    
    #define access(path, mode) _access(path, mode)
    #define rename(oldpath, newpath) rename(oldpath, newpath)
    #define unlink(path) _unlink(path)
    
    struct stat {
        off_t st_size;
    };
    
    static int lseek(int fd, off_t offset, int whence) {
        return SetFilePointer((HANDLE)fd_map[fd], offset, NULL, whence);
    }
    
    static int stat(const char* path, struct stat* buf) {
        WIN32_FILE_ATTRIBUTE_DATA attr;
        if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attr)) {
            return -1;
        }
        buf->st_size = (attr.nFileSizeHigh << 32) | attr.nFileSizeLow;
        return 0;
    }
    
    static int fstat(int fd, struct stat* buf) {
        HANDLE handle = (HANDLE)fd_map[fd];
        LARGE_INTEGER size;
        if (!GetFileSizeEx(handle, &size)) {
            return -1;
        }
        buf->st_size = size.QuadPart;
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

// 文件描述符映射表
static platform_handle fd_map[1024] = {0};
static int next_fd = 3; // 0, 1, 2 分别对应 stdin, stdout, stderr

// 初始化系统调用表
void init_syscall_table(void) {
    // 初始化文件描述符映射
    fd_map[0] = 0; // stdin
    fd_map[1] = 1; // stdout
    fd_map[2] = 2; // stderr
    
    // 注册 BSD 系统调用
    syscall_table[SYS_open] = (syscall_handler_t)sys_open;
    syscall_table[SYS_close] = (syscall_handler_t)sys_close;
    syscall_table[SYS_read] = (syscall_handler_t)sys_read;
    syscall_table[SYS_write] = (syscall_handler_t)sys_write;
    syscall_table[SYS_lseek] = (syscall_handler_t)sys_lseek;
    syscall_table[SYS_stat] = (syscall_handler_t)sys_stat;
    syscall_table[SYS_fstat] = (syscall_handler_t)sys_fstat;
    syscall_table[SYS_getpid] = (syscall_handler_t)sys_getpid;
    syscall_table[SYS_getppid] = (syscall_handler_t)sys_getppid;
    syscall_table[SYS_getuid] = (syscall_handler_t)sys_getuid;
    syscall_table[SYS_geteuid] = (syscall_handler_t)sys_geteuid;
    syscall_table[SYS_getgid] = (syscall_handler_t)sys_getgid;
    syscall_table[SYS_getegid] = (syscall_handler_t)sys_getegid;
    syscall_table[SYS_brk] = (syscall_handler_t)sys_brk;
    syscall_table[SYS_mmap] = (syscall_handler_t)sys_mmap;
    syscall_table[SYS_munmap] = (syscall_handler_t)sys_munmap;
    
    // 注册 Mach 系统调用
    syscall_table[SYS_mach_msg] = (syscall_handler_t)sys_mach_msg;
    syscall_table[SYS_thread_self] = (syscall_handler_t)sys_thread_self;
    syscall_table[SYS_task_self] = (syscall_handler_t)sys_task_self;
}

// 处理系统调用
int handle_syscall(int syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    if (syscall_num < 0 || syscall_num >= 256 || !syscall_table[syscall_num]) {
        printf("Error: Unknown syscall %d\n", syscall_num);
        return -1;
    }
    
    return syscall_table[syscall_num](arg1, arg2, arg3, arg4, arg5, arg6);
}

// 打开文件
int sys_open(const char* path, int flags, int mode) {
    platform_handle handle = platform_open(path, flags, mode);
    if (!handle) {
        return -1;
    }
    
    // 分配文件描述符
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
    fd_map[fd] = NULL;
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
    
    // 简化实现
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

// 获取进程 ID
pid_t sys_getpid(void) {
    return getpid();
}

// 获取父进程 ID
pid_t sys_getppid(void) {
    return getppid();
}

// 获取用户 ID
uid_t sys_getuid(void) {
    return getuid();
}

// 获取有效用户 ID
uid_t sys_geteuid(void) {
    return geteuid();
}

// 获取组 ID
gid_t sys_getgid(void) {
    return getgid();
}

// 获取有效组 ID
gid_t sys_getegid(void) {
    return getegid();
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

// IO 控制
int sys_ioctl(int fd, unsigned long request, void* arg) {
    // 简化实现
    return 0;
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

// 重命名文件
int sys_rename(const char* oldpath, const char* newpath) {
    return rename(oldpath, newpath);
}

// 删除文件
int sys_unlink(const char* path) {
    return unlink(path);
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
