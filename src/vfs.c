#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <Windows.h>
    
    #define F_OK 0
    #define R_OK 4
    #define W_OK 2
    #define X_OK 1
    
    #define getcwd(buf, size) _getcwd(buf, size)
    #define mkdir(path, mode) _mkdir(path)
    #define access(path, mode) _access(path, mode)
    #define unlink(path) _unlink(path)
    
    typedef struct {
        HANDLE hFind;
        WIN32_FIND_DATAA findData;
        bool first;
    } DIR;
    
    static DIR* opendir(const char* path) {
        char search_path[MAX_PATH];
        sprintf(search_path, "%s/*", path);
        
        DIR* dir = (DIR*)malloc(sizeof(DIR));
        if (!dir) return NULL;
        
        dir->hFind = FindFirstFileA(search_path, &dir->findData);
        if (dir->hFind == INVALID_HANDLE_VALUE) {
            free(dir);
            return NULL;
        }
        dir->first = true;
        return dir;
    }
    
    static struct dirent {
        char d_name[256];
    };
    
    static struct dirent* readdir(DIR* dir) {
        static struct dirent entry;
        
        if (dir->first) {
            dir->first = false;
        } else {
            if (!FindNextFileA(dir->hFind, &dir->findData)) {
                return NULL;
            }
        }
        
        strncpy(entry.d_name, dir->findData.cFileName, sizeof(entry.d_name) - 1);
        entry.d_name[sizeof(entry.d_name) - 1] = '\0';
        return &entry;
    }
    
    static void closedir(DIR* dir) {
        FindClose(dir->hFind);
        free(dir);
    }
    
    static void rewinddir(DIR* dir) {
        // 简化实现
    }
#else
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
        
        // 添加基本的路径映射
        vfs_add_mapping("/usr/lib", lib_path);
        vfs_add_mapping("/usr/local/lib", lib_path);
        vfs_add_mapping("/Library/Frameworks", lib_path);
    }
    printf("VFS initialized\n");
}

// 关闭虚拟文件系统
void vfs_cleanup(void) {
    vfs_entry* current = vfs_root;
    while (current) {
        vfs_entry* next = (vfs_entry*)current->next;
        free(current->virtual_path);
        free(current->physical_path);
        free(current);
        current = next;
    }
    vfs_root = NULL;
    printf("VFS cleanup complete\n");
}

// 添加虚拟路径映射
int vfs_add_mapping(const char* virtual_path, const char* physical_path) {
    vfs_entry* entry = (vfs_entry*)malloc(sizeof(vfs_entry));
    if (!entry) {
        return -1;
    }
    
    entry->virtual_path = strdup(virtual_path);
    entry->physical_path = strdup(physical_path);
    entry->next = vfs_root;
    vfs_root = entry;
    
    return 0;
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
        current = (vfs_entry*)current->next;
    }
    
    // 没有找到映射，返回原路径
    return virtual_path;
}

// 检查文件是否存在
bool vfs_file_exists(const char* virtual_path) {
    const char* physical_path = vfs_resolve_path(virtual_path);
    if (!physical_path) {
        return false;
    }
    
    return (access(physical_path, F_OK) == 0);
}

// 列出目录内容
int vfs_list_directory(const char* virtual_path, char*** entries, int* count) {
    const char* physical_path = vfs_resolve_path(virtual_path);
    if (!physical_path) {
        return -1;
    }
    
    DIR* dir = opendir(physical_path);
    if (!dir) {
        return -1;
    }
    
    // 计算条目数量
    *count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        (*count)++;
    }
    
    rewinddir(dir);
    
    // 重新遍历并收集条目
    *entries = (char**)malloc(sizeof(char*) * (*count));
    if (!*entries) {
        closedir(dir);
        return -1;
    }
    
    int i = 0;
    while ((entry = readdir(dir)) != NULL) {
        (*entries)[i] = strdup(entry->d_name);
        i++;
    }
    
    closedir(dir);
    return 0;
}

// 创建目录
int vfs_mkdir(const char* virtual_path, int mode) {
    const char* physical_path = vfs_resolve_path(virtual_path);
    if (!physical_path) {
        return -1;
    }
    
    return mkdir(physical_path, mode) == 0 ? 0 : -1;
}

// 删除文件
int vfs_unlink(const char* virtual_path) {
    const char* physical_path = vfs_resolve_path(virtual_path);
    if (!physical_path) {
        return -1;
    }
    
    return unlink(physical_path) == 0 ? 0 : -1;
}

// 重命名文件
int vfs_rename(const char* old_path, const char* new_path) {
    const char* old_physical = vfs_resolve_path(old_path);
    const char* new_physical = vfs_resolve_path(new_path);
    
    if (!old_physical || !new_physical) {
        return -1;
    }
    
    return rename(old_physical, new_physical) == 0 ? 0 : -1;
}
