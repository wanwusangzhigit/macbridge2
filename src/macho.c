#include "macho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

// 解析 Mach-O 头部
bool parse_macho_header(const char* filename, struct mach_header** header, bool* is_64bit) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return false;
    }

    // 读取魔数
    uint32_t magic;
    if (fread(&magic, sizeof(magic), 1, file) != 1) {
        fclose(file);
        return false;
    }

    *is_64bit = false;
    if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64) {
        *is_64bit = true;
    } else if (magic != MH_MAGIC && magic != MH_CIGAM) {
        fclose(file);
        return false;
    }

    // 分配头部内存
    size_t header_size = *is_64bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header);
    *header = (struct mach_header*)malloc(header_size);
    if (!*header) {
        fclose(file);
        return false;
    }

    // 重新读取头部
    fseek(file, 0, SEEK_SET);
    if (fread(*header, header_size, 1, file) != 1) {
        free(*header);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

// 映射 Mach-O 段到内存
bool map_macho_segments(const char* filename, struct mach_header* header, bool is_64bit, void** base_address) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return false;
    }

    // 计算文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 分配内存
    *base_address = VirtualAlloc(NULL, file_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!*base_address) {
        fclose(file);
        return false;
    }

    // 读取整个文件到内存
    if (fread(*base_address, file_size, 1, file) != 1) {
        VirtualFree(*base_address, 0, MEM_RELEASE);
        fclose(file);
        return false;
    }

    // 处理加载命令
    uint8_t* cmd_ptr = (uint8_t*)header + (*is_64bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header));
    for (uint32_t i = 0; i < header->ncmds; i++) {
        struct load_command* cmd = (struct load_command*)cmd_ptr;
        if (cmd->cmd == LC_SEGMENT || cmd->cmd == LC_SEGMENT_64) {
            if (is_64bit) {
                struct segment_command_64* seg = (struct segment_command_64*)cmd;
                // 处理 64 位段
                printf("Segment: %s, vmaddr: 0x%llx, vmsize: 0x%llx\n", 
                       seg->segname, seg->vmaddr, seg->vmsize);
            } else {
                struct segment_command* seg = (struct segment_command*)cmd;
                // 处理 32 位段
                printf("Segment: %s, vmaddr: 0x%x, vmsize: 0x%x\n", 
                       seg->segname, seg->vmaddr, seg->vmsize);
            }
        }
        cmd_ptr += cmd->cmdsize;
    }

    fclose(file);
    return true;
}

// 查找 main 函数
void* find_main_function(const char* filename, struct mach_header* header, bool is_64bit, void* base_address) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }

    void* main_addr = NULL;
    uint8_t* cmd_ptr = (uint8_t*)header + (*is_64bit ? sizeof(struct mach_header_64) : sizeof(struct mach_header));
    for (uint32_t i = 0; i < header->ncmds; i++) {
        struct load_command* cmd = (struct load_command*)cmd_ptr;
        if (cmd->cmd == LC_MAIN) {
            struct entry_point_command* entry = (struct entry_point_command*)cmd;
            main_addr = (uint8_t*)base_address + entry->entryoff;
            printf("Found main function at offset: 0x%llx\n", entry->entryoff);
            break;
        }
        cmd_ptr += cmd->cmdsize;
    }

    fclose(file);
    return main_addr;
}

// 释放内存映射
void unmap_macho_segments(void* base_address, struct mach_header* header, bool is_64bit) {
    if (base_address) {
        VirtualFree(base_address, 0, MEM_RELEASE);
    }
    if (header) {
        free(header);
    }
}
