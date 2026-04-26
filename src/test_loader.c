#include "macho.h"
#include "syscall.h"
#include "vfs.h"
#include "dyld.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <macho_executable>\n", argv[0]);
        return 1;
    }

    // 初始化各个子系统
    printf("Initializing subsystems...\n");
    init_syscall_table();
    vfs_init();
    dyld_init();

    const char* filename = argv[1];
    struct mach_header* header = NULL;
    bool is_64bit = false;

    // 解析 Mach-O 头部
    if (!parse_macho_header(filename, &header, &is_64bit)) {
        printf("Failed to parse Mach-O header\n");
        goto cleanup;
    }

    printf("Mach-O Header Info:\n");
    printf("Magic: 0x%x\n", header->magic);
    printf("CPU Type: 0x%x\n", header->cputype);
    printf("File Type: 0x%x\n", header->filetype);
    printf("Number of commands: %d\n", header->ncmds);
    printf("Size of commands: %d\n", header->sizeofcmds);
    printf("Is 64-bit: %s\n", is_64bit ? "Yes" : "No");

    // 映射段到内存
    void* base_address = NULL;
    if (!map_macho_segments(filename, header, is_64bit, &base_address)) {
        printf("Failed to map Mach-O segments\n");
        goto cleanup;
    }

    // 查找 main 函数
    void* main_addr = find_main_function(filename, header, is_64bit, base_address);
    if (main_addr) {
        printf("Main function found at address: %p\n", main_addr);
        
        // 执行 main 函数
        printf("Executing main function...\n");
        int result = execute_main_function(main_addr, argc, argv);
        printf("Main function returned: %d\n", result);
    } else {
        printf("Main function not found\n");
    }

cleanup:
    // 释放资源
    if (base_address && header) {
        unmap_macho_segments(base_address, header, is_64bit);
    } else if (header) {
        free(header);
    }
    
    // 清理各个子系统
    dyld_cleanup();
    vfs_cleanup();

    return 0;
}
