#pragma once

#include <stdint.h>
#include "platform.h"

// 动态库结构
typedef struct dyld_image {
    char* path;           // 库路径
    void* base_address;   // 加载基地址
    size_t size;          // 大小
    struct dyld_image* next;
} dyld_image;

// 符号结构
typedef struct dyld_symbol {
    char* name;           // 符号名称
    void* address;        // 符号地址
    struct dyld_symbol* next;
} dyld_symbol;

// 初始化 dyld 模拟器
void dyld_init(void);

// 关闭 dyld 模拟器
void dyld_cleanup(void);

// 添加符号到符号表
void dyld_add_symbol(const char* name, void* address);

// 加载动态库
void* dyld_load_image(const char* path);

// 卸载动态库
void dyld_unload_image(void* image);

// 查找符号
void* dyld_lookup_symbol(const char* name);

// 处理延迟绑定
void dyld_process_lazy_bind(void* image);

// 处理重定位
void dyld_process_relocations(void* image);

// 模拟 dlopen
void* dyld_dlopen(const char* path, int flags);

// 模拟 dlsym
void* dyld_dlsym(void* handle, const char* symbol);

// 模拟 dlclose
int dyld_dlclose(void* handle);

// 模拟 dlerror
const char* dyld_dlerror(void);
