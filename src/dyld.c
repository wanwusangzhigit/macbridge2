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
    // 初始化符号表 - 添加标准库函数
    dyld_add_symbol("printf", (void*)printf);
    dyld_add_symbol("fprintf", (void*)fprintf);
    dyld_add_symbol("sprintf", (void*)sprintf);
    dyld_add_symbol("snprintf", (void*)snprintf);
    
    // 内存管理
    dyld_add_symbol("malloc", (void*)malloc);
    dyld_add_symbol("free", (void*)free);
    dyld_add_symbol("calloc", (void*)calloc);
    dyld_add_symbol("realloc", (void*)realloc);
    
    // 字符串操作
    dyld_add_symbol("strcpy", (void*)strcpy);
    dyld_add_symbol("strncpy", (void*)strncpy);
    dyld_add_symbol("strlen", (void*)strlen);
    dyld_add_symbol("strcmp", (void*)strcmp);
    dyld_add_symbol("strncmp", (void*)strncmp);
    dyld_add_symbol("strcat", (void*)strcat);
    dyld_add_symbol("strncat", (void*)strncat);
    dyld_add_symbol("strstr", (void*)strstr);
    dyld_add_symbol("strchr", (void*)strchr);
    dyld_add_symbol("strrchr", (void*)strrchr);
    
    // 内存操作
    dyld_add_symbol("memcpy", (void*)memcpy);
    dyld_add_symbol("memset", (void*)memset);
    dyld_add_symbol("memmove", (void*)memmove);
    dyld_add_symbol("memcmp", (void*)memcmp);
    
    // 文件操作
    dyld_add_symbol("fopen", (void*)fopen);
    dyld_add_symbol("fclose", (void*)fclose);
    dyld_add_symbol("fread", (void*)fread);
    dyld_add_symbol("fwrite", (void*)fwrite);
    dyld_add_symbol("fseek", (void*)fseek);
    dyld_add_symbol("ftell", (void*)ftell);
    dyld_add_symbol("rewind", (void*)rewind);
    dyld_add_symbol("feof", (void*)feof);
    
    // 输入输出
    dyld_add_symbol("putchar", (void*)putchar);
    dyld_add_symbol("puts", (void*)puts);
    dyld_add_symbol("getchar", (void*)getchar);
    dyld_add_symbol("fgets", (void*)fgets);
    
    // 标准函数
    dyld_add_symbol("atoi", (void*)atoi);
    dyld_add_symbol("atol", (void*)atol);
    dyld_add_symbol("atof", (void*)atof);
    dyld_add_symbol("exit", (void*)exit);
    dyld_add_symbol("abort", (void*)abort);
    
    printf("dyld initialized with standard library symbols\n");
}

// 关闭 dyld 模拟器
void dyld_cleanup(void) {
    // 卸载所有动态库
    dyld_image* current = dyld_images;
    while (current) {
        dyld_image* next = (dyld_image*)current->next;
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
        dyld_symbol* sym_next = (dyld_symbol*)sym_current->next;
        free(sym_current->name);
        free(sym_current);
        sym_current = sym_next;
    }
    dyld_symbols = NULL;
    printf("dyld cleanup complete\n");
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
    printf("dyld: Loading library %s\n", path);
    
    // 解析虚拟路径
    const char* physical_path = vfs_resolve_path(path);
    if (!physical_path) {
        snprintf(dyld_error, sizeof(dyld_error), "Cannot resolve path: %s", path);
        return NULL;
    }
    
    // 检查文件是否存在
    if (!vfs_file_exists(path)) {
        snprintf(dyld_error, sizeof(dyld_error), "File not found: %s", path);
        return NULL;
    }
    
    // 解析 Mach-O 头部
    struct mach_header* header = NULL;
    bool is_64bit = false;
    if (!parse_macho_header(physical_path, &header, &is_64bit)) {
        snprintf(dyld_error, sizeof(dyld_error), "Invalid Mach-O file: %s", path);
        return NULL;
    }
    
    // 映射段到内存
    void* base_address = NULL;
    if (!map_macho_segments(physical_path, header, is_64bit, &base_address)) {
        snprintf(dyld_error, sizeof(dyld_error), "Failed to map segments: %s", path);
        free(header);
        return NULL;
    }
    
    // 创建动态库结构
    dyld_image* image = (dyld_image*)malloc(sizeof(dyld_image));
    if (!image) {
        snprintf(dyld_error, sizeof(dyld_error), "Out of memory");
        unmap_macho_segments(base_address, header, is_64bit);
        free(header);
        return NULL;
    }
    
    memset(image, 0, sizeof(dyld_image));
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
    printf("dyld: Loaded library %s at %p\n", path, base_address);
    return image;
}

// 卸载动态库
void dyld_unload_image(void* image_ptr) {
    dyld_image* image = (dyld_image*)image_ptr;
    if (!image) {
        return;
    }
    
    printf("dyld: Unloading library %s\n", image->path);
    
    // 从链表中移除
    if (dyld_images == image) {
        dyld_images = (dyld_image*)image->next;
    } else {
        dyld_image* current = dyld_images;
        while (current && current->next != image) {
            current = (dyld_image*)current->next;
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
        current = (dyld_symbol*)current->next;
    }
    
    // 遍历动态库
    dyld_image* image_current = dyld_images;
    while (image_current) {
        // 这里应该解析动态库的符号表
        // 简化实现，暂时只搜索内置符号表
        image_current = (dyld_image*)image_current->next;
    }
    
    snprintf(dyld_error, sizeof(dyld_error), "Symbol not found: %s", name);
    return NULL;
}

// 处理延迟绑定
void dyld_process_lazy_bind(void* image_ptr) {
    dyld_image* image = (dyld_image*)image_ptr;
    if (image) {
        // 简化实现 - 真实的实现需要解析 Mach-O 的 lazy bind 信息
        printf("dyld: Processed lazy bind for %s\n", image->path);
    }
}

// 处理重定位
void dyld_process_relocations(void* image_ptr) {
    dyld_image* image = (dyld_image*)image_ptr;
    if (image) {
        // 简化实现 - 真实的实现需要解析 Mach-O 的 relocation 信息
        printf("dyld: Processed relocations for %s\n", image->path);
    }
}

// 模拟 dlopen
void* dyld_dlopen(const char* path, int flags) {
    (void)flags; // 忽略标志位，简化实现
    printf("dyld: dlopen(%s)\n", path);
    return dyld_load_image(path);
}

// 模拟 dlsym
void* dyld_dlsym(void* handle, const char* symbol) {
    if (handle == NULL) {
        printf("dyld: dlsym(RTLD_DEFAULT, %s)\n", symbol);
        return dyld_lookup_symbol(symbol);
    }
    printf("dyld: dlsym(%p, %s)\n", handle, symbol);
    return dyld_lookup_symbol(symbol);
}

// 模拟 dlclose
int dyld_dlclose(void* handle) {
    printf("dyld: dlclose(%p)\n", handle);
    dyld_unload_image(handle);
    return 0;
}

// 模拟 dlerror
const char* dyld_dlerror(void) {
    return dyld_error;
}
