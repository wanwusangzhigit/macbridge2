#pragma once

#include <stdint.h>
#include <stdbool.h>

// 平台抽象层
#ifdef _WIN32
    #include <Windows.h>
    typedef HANDLE platform_handle;
    typedef DWORD platform_size_t;
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <fcntl.h>
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
