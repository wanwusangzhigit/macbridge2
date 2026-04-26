#include "platform.h"
#include <stdlib.h>
#include <string.h>

// 内存分配函数
void* platform_alloc(size_t size) {
    return malloc(size);
}

// 内存释放函数
void platform_free(void* ptr) {
    free(ptr);
}

// 文件打开函数
platform_handle platform_open(const char* path, int flags, int mode) {
#ifdef _WIN32
    DWORD access = 0;
    DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD creation = OPEN_EXISTING;
    
    if (flags & O_RDONLY) access |= GENERIC_READ;
    if (flags & O_WRONLY) access |= GENERIC_WRITE;
    if (flags & O_RDWR) access |= GENERIC_READ | GENERIC_WRITE;
    
    if (flags & O_CREAT) creation = CREATE_NEW;
    if (flags & O_TRUNC) creation = CREATE_ALWAYS;
    if (flags & O_APPEND) creation = OPEN_ALWAYS;
    
    return CreateFileA(path, access, share, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
#else
    return open(path, flags, mode);
#endif
}

// 文件关闭函数
int platform_close(platform_handle handle) {
#ifdef _WIN32
    return CloseHandle(handle) ? 0 : -1;
#else
    return close(handle);
#endif
}

// 文件读取函数
ssize_t platform_read(platform_handle handle, void* buf, size_t count) {
#ifdef _WIN32
    DWORD bytes_read;
    if (!ReadFile(handle, buf, count, &bytes_read, NULL)) {
        return -1;
    }
    return bytes_read;
#else
    return read(handle, buf, count);
#endif
}

// 文件写入函数
ssize_t platform_write(platform_handle handle, const void* buf, size_t count) {
#ifdef _WIN32
    DWORD bytes_written;
    if (!WriteFile(handle, buf, count, &bytes_written, NULL)) {
        return -1;
    }
    return bytes_written;
#else
    return write(handle, buf, count);
#endif
}

// 内存映射函数
void* platform_mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
#ifdef _WIN32
    DWORD protect = 0;
    if (prot & PROT_READ) protect |= PAGE_READONLY;
    if (prot & PROT_WRITE) protect |= PAGE_READWRITE;
    if (prot & PROT_EXEC) protect |= PAGE_EXECUTE;
    
    return VirtualAlloc(addr, length, MEM_COMMIT | MEM_RESERVE, protect);
#else
    return mmap(addr, length, prot, flags, fd, offset);
#endif
}

// 内存解除映射函数
int platform_munmap(void* addr, size_t length) {
#ifdef _WIN32
    return VirtualFree(addr, length, MEM_DECOMMIT) ? 0 : -1;
#else
    return munmap(addr, length);
#endif
}

// 内存保护函数
int platform_protect(void* addr, size_t length, int prot) {
#ifdef _WIN32
    DWORD protect = 0;
    if (prot & PROT_READ) protect |= PAGE_READONLY;
    if (prot & PROT_WRITE) protect |= PAGE_READWRITE;
    if (prot & PROT_EXEC) protect |= PAGE_EXECUTE;
    
    DWORD old_protect;
    return VirtualProtect(addr, length, protect, &old_protect) ? 0 : -1;
#else
    return mprotect(addr, length, prot);
#endif
}

// 路径处理函数
const char* platform_resolve_path(const char* path) {
    // 简化实现，直接返回原路径
    return path;
}
