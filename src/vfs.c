#include "vfs.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

// 虚拟文件系统根节点
static vfs_entry* vfs_root = NULL;

// 初始化虚拟文件系统
void vfs_init(void) {
    // 创建默认的路径映射
    char current_path[4096];
    if (getcwd(current_path, sizeof(current_path))) {
        // 创建默认的目录结构
        char lib_path[4100];
        snprintf(lib_path, sizeof(lib_path), "%s/lib", current_path);
        mkdir(lib_path, 0755);
        
        char app_path[4100];
        snprintf(app_path, sizeof(app_path), "%s/Applications", current_path);
        mkdir(app_path, 0755);
    }
}

// 清理虚拟文件系统
void vfs_cleanup(void) {
    // 释放路径映射表
    vfs_entry* current = vfs_root;
    while (current) {
        vfs_entry* next = current->next;
        free(current->virtual_path);
        free(current->physical_path);
        free(current);
        current = next;
    }
    vfs_root = NULL;
}

// 添加路径映射
bool vfs_add_mapping(const char* virtual_path, const char* physical_path) {
    if (!virtual_path || !physical_path) {
        return false;
    }
    
    vfs_entry* entry = (vfs_entry*)malloc(sizeof(vfs_entry));
    if (!entry) {
        return false;
    }
    
    entry->virtual_path = strdup(virtual_path);
    entry->physical_path = strdup(physical_path);
    entry->next = vfs_root;
    vfs_root = entry;
    
    return true;
}

// 移除路径映射
bool vfs_remove_mapping(const char* virtual_path) {
    if (!virtual_path) {
        return false;
    }
    
    vfs_entry* current = vfs_root;
    vfs_entry* prev = NULL;
    
    while (current) {
        if (strcmp(current->virtual_path, virtual_path) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                vfs_root = current->next;
            }
            free(current->virtual_path);
            free(current->physical_path);
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    
    return false;
}

// 解析虚拟路径到物理路径
const char* vfs_resolve_path(const char* virtual_path) {
    if (!virtual_path) {
        return NULL;
    }
    
    // 检查是否是绝对路径
    if (virtual_path[0] != '/') {
        return virtual_path;
    }
    
    // 遍历映射表
    vfs_entry* current = vfs_root;
    while (current) {
        size_t virt_len = strlen(current->virtual_path);
        if (strncmp(virtual_path, current->virtual_path, virt_len) == 0) {
            // 找到匹配的映射
            static char resolved_path[4096];
            const char* rest = virtual_path + virt_len;
            
            // 处理路径拼接
            if (*rest == '/') {
                rest++;
            }
            
            if (snprintf(resolved_path, sizeof(resolved_path), "%s/%s", current->physical_path, rest) >= (int)sizeof(resolved_path)) {
                fprintf(stderr, "Resolved path too long: %s/%s\n", current->physical_path, rest);
                return NULL;
            }
            
            // 转换路径分隔符
            for (char* p = resolved_path; *p; p++) {
                if (*p == '\\') {
                    *p = '/';
                }
            }
            
            return resolved_path;
        }
        current = current->next;
    }
    
    // 没有找到映射，返回原路径
    return virtual_path;
}

// 列出所有映射
void vfs_list_mappings(void) {
    vfs_entry* current = vfs_root;
    int count = 0;
    
    printf("\nVirtual File System Mappings:\n");
    printf("────────────────────────────────────────\n");
    
    while (current) {
        printf("  %s -> %s\n", current->virtual_path, current->physical_path);
        current = current->next;
        count++;
    }
    
    if (count == 0) {
        printf("  (no mappings)\n");
    }
    printf("────────────────────────────────────────\n");
    printf("Total: %d mappings\n\n", count);
}
