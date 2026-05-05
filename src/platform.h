#pragma once

#include <stdint.h>
#include <stdbool.h>

// 平台抽象层
#ifdef _WIN32
    #include <Windows.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <direct.h>
    
    typedef HANDLE platform_handle;
    typedef DWORD platform_size_t;
    #ifndef ssize_t
    typedef LONG ssize_t;
    #endif
    #ifndef pid_t
    typedef int pid_t;
    #endif
    
    typedef struct _WIN32_DIR {
        HANDLE hFind;
        WIN32_FIND_DATA FindData;
        char entry_name[MAX_PATH];
    } DIR;
    
    struct dirent {
        char d_name[MAX_PATH];
    };
    
    DIR* opendir(const char* name);
    struct dirent* readdir(DIR* dirp);
    int closedir(DIR* dirp);
    
    #define PROT_READ  0x1
    #define PROT_WRITE 0x2
    #define PROT_EXEC  0x4
    
    #define O_RDONLY   0x0000
    #define O_WRONLY   0x0001
    #define O_RDWR     0x0002
    #define O_CREAT    0x0100
    #define O_TRUNC    0x0200
    #define O_APPEND   0x0400
    
    // Windows 下的 stat 结构和相关宏的补充定义
    #define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
    #define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
    
    // 使用 Windows 的 _stat 结构
    #define stat _stat
    #define mkdir(path, mode) _mkdir(path)
    #define unlink(path) _unlink(path)
    #define access(path, mode) _access(path, mode)
    #define getcwd(buf, size) _getcwd(buf, size)
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <sys/stat.h>
    typedef int platform_handle;
    typedef size_t platform_size_t;
#endif

// 内存分配函数
void* platform_alloc(size_t size);
void platform_free(void* ptr);

// 文件操作函数
platform_handle platform_open(const char* path, int flags, int mode);
int platform_close(platform_handle handle);
ssize_t platform_read(platform_handle handle, void* buf, size_t count);
ssize_t platform_write(platform_handle handle, const void* buf, size_t count);

// 内存映射函数
void* platform_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
int platform_munmap(void* addr, size_t length);

// 内存保护函数
int platform_protect(void* addr, size_t length, int prot);

// 路径处理函数
const char* platform_resolve_path(const char* path);
