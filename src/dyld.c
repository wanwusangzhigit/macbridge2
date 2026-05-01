#include "dyld.h"
#include "vfs.h"
#include "macho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 加载的动态库列表
static dyld_image* dyld_images = NULL;

// 全局符号表
static dyld_symbol* dyld_symbols = NULL;

// 错误信息
static char dyld_error[256] = "";

// 初始化 dyld 模拟器
void dyld_init(void) {
    // 初始化符号表
    // 添加基本的系统符号
    dyld_add_symbol("printf", (void*)printf);
    dyld_add_symbol("malloc", (void*)malloc);
    dyld_add_symbol("free", (void*)free);
    dyld_add_symbol("strcpy", (void*)strcpy);
    dyld_add_symbol("strlen", (void*)strlen);
}

// 关闭 dyld 模拟器
void dyld_cleanup(void) {
    // 卸载所有动态库
    dyld_image* current = dyld_images;
    while (current) {
        dyld_image* next = current->next;
        // 释放内存
        if (current->base_address) {
            platform_free(current->base_address);
        }
        free(current->path);
        free(current);
        current = next;
    }
    dyld_images = NULL;
    
    // 清理符号表
    dyld_symbol* sym_current = dyld_symbols;
    while (sym_current) {
        dyld_symbol* sym_next = sym_current->next;
        free(sym_current->name);
        free(sym_current);
        sym_current = sym_next;
    }
    dyld_symbols = NULL;
}

// 添加符号到符号表
void dyld_add_symbol(const char* name, void* address) {
    dyld_symbol* symbol = (dyld_symbol*)malloc(sizeof(dyld_symbol));
    if (!symbol) {
        return;
    }
    
    symbol->name = strdup(name);
    symbol->address = address;
    symbol->next = dyld_symbols;
    dyld_symbols = symbol;
}

// 加载动态库
void* dyld_load_image(const char* path) {
    // 解析虚拟路径
    const char* physical_path = vfs_resolve_path(path);
    if (!physical_path) {
        sprintf(dyld_error, "Cannot resolve path: %s", path);
        return NULL;
    }
    
    // 检查文件是否存在
    if (!vfs_file_exists(path)) {
        sprintf(dyld_error, "File not found: %s", path);
        return NULL;
    }
    
    // 解析 Mach-O 头部
    struct mach_header* header = NULL;
    bool is_64bit = false;
    if (!parse_macho_header(physical_path, &header, &is_64bit)) {
        sprintf(dyld_error, "Invalid Mach-O file: %s", path);
        return NULL;
    }
    
    // 映射段到内存
    void* base_address = NULL;
    if (!map_macho_segments(physical_path, header, is_64bit, &base_address)) {
        sprintf(dyld_error, "Failed to map segments: %s", path);
        free(header);
        return NULL;
    }
    
    // 创建动态库结构
    dyld_image* image = (dyld_image*)malloc(sizeof(dyld_image));
    if (!image) {
        sprintf(dyld_error, "Out of memory");
        unmap_macho_segments(base_address, header, is_64bit);
        free(header);
        return NULL;
    }
    
    image->path = strdup(path);
    image->base_address = base_address;
    image->size = 0; // 暂时设置为 0
    image->next = dyld_images;
    dyld_images = image;
    
    // 处理重定位
    dyld_process_relocations(image);
    
    // 处理延迟绑定
    dyld_process_lazy_bind(image);
    
    free(header);
    return image;
}

// 卸载动态库
void dyld_unload_image(void* image_ptr) {
    dyld_image* image = (dyld_image*)image_ptr;
    if (!image) {
        return;
    }
    
    // 从链表中移除
    if (dyld_images == image) {
        dyld_images = image->next;
    } else {
        dyld_image* current = dyld_images;
        while (current && current->next != image) {
            current = current->next;
        }
        if (current) {
            current->next = image->next;
        }
    }
    
    // 释放内存
    if (image->base_address) {
        platform_free(image->base_address);
    }
    free(image->path);
    free(image);
}

// 查找符号
void* dyld_lookup_symbol(const char* name) {
    // 遍历符号表
    dyld_symbol* current = dyld_symbols;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->address;
        }
        current = current->next;
    }
    
    // 遍历动态库
    dyld_image* image_current = dyld_images;
    while (image_current) {
        // 这里应该解析动态库的符号表
        // 简化实现，返回 NULL
        image_current = image_current->next;
    }
    
    return NULL;
}

// 处理延迟绑定
void dyld_process_lazy_bind(void* image_ptr) {
    // 简化实现
    dyld_image* image = (dyld_image*)image_ptr;
    printf("Processing lazy bind for %s\n", image->path);
}

// 处理重定位
void dyld_process_relocations(void* image_ptr) {
    // 简化实现
    dyld_image* image = (dyld_image*)image_ptr;
    printf("Processing relocations for %s\n", image->path);
}

// 模拟 dlopen
void* dyld_dlopen(const char* path, int flags) {
    return dyld_load_image(path);
}

// 模拟 dlsym
void* dyld_dlsym(void* handle, const char* symbol) {
    return dyld_lookup_symbol(symbol);
}

// 模拟 dlclose
int dyld_dlclose(void* handle) {
    dyld_unload_image(handle);
    return 0;
}

// 模拟 dlerror
const char* dyld_dlerror(void) {
    return dyld_error;
}
