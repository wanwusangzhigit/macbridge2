#pragma once

#include <stdint.h>
#include "platform.h"

// 虚拟文件系统结构
typedef struct {
    char* virtual_path;   // 虚拟路径（如 /usr/lib/libSystem.dylib）
    char* physical_path;  // 物理路径（如 C:\WinDarling\lib\libSystem.dylib）
    struct vfs_entry* next;
} vfs_entry;

// 初始化虚拟文件系统
void vfs_init(void);

// 关闭虚拟文件系统
void vfs_cleanup(void);

// 添加虚拟路径映射
int vfs_add_mapping(const char* virtual_path, const char* physical_path);

// 解析虚拟路径到物理路径
const char* vfs_resolve_path(const char* virtual_path);

// 检查文件是否存在
bool vfs_file_exists(const char* virtual_path);

// 列出目录内容
int vfs_list_directory(const char* virtual_path, char*** entries, int* count);

// 创建目录
int vfs_mkdir(const char* virtual_path, int mode);

// 删除文件
int vfs_unlink(const char* virtual_path);

// 重命名文件
int vfs_rename(const char* old_path, const char* new_path);
