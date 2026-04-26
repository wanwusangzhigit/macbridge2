#pragma once

#include <stdint.h>

// Mach-O 魔数
#define MH_MAGIC    0xfeedface
#define MH_CIGAM    0xcefaedfe
#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_64 0xcffaedfe

// CPU 类型
#define CPU_TYPE_X86        7
#define CPU_TYPE_X86_64     (CPU_TYPE_X86 | 0x1000000)
#define CPU_TYPE_ARM        12
#define CPU_TYPE_ARM64      (CPU_TYPE_ARM | 0x1000000)

// 文件类型
#define MH_EXECUTE      0x2
#define MH_DYLIB        0x6
#define MH_BUNDLE       0x8

// 加载命令类型
#define LC_SEGMENT      0x1
#define LC_SEGMENT_64   0x19
#define LC_LOAD_DYLIB   0xc
#define LC_MAIN         0x28
#define LC_CODE_SIGNATURE 0x1d

// 结构体定义
struct mach_header {
    uint32_t magic;
    cpu_type_t cputype;
    cpu_subtype_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
};

struct mach_header_64 {
    uint32_t magic;
    cpu_type_t cputype;
    cpu_subtype_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
};

struct load_command {
    uint32_t cmd;
    uint32_t cmdsize;
};

struct segment_command {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint32_t vmaddr;
    uint32_t vmsize;
    uint32_t fileoff;
    uint32_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
};

struct segment_command_64 {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
};

struct section {
    char sectname[16];
    char segname[16];
    uint32_t addr;
    uint32_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
};

struct section_64 {
    char sectname[16];
    char segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
};

struct entry_point_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint64_t entryoff;
    uint64_t stacksize;
};

// 函数声明
bool parse_macho_header(const char* filename, struct mach_header** header, bool* is_64bit);
bool map_macho_segments(const char* filename, struct mach_header* header, bool is_64bit, void** base_address);
void* find_main_function(const char* filename, struct mach_header* header, bool is_64bit, void* base_address);
void unmap_macho_segments(void* base_address, struct mach_header* header, bool is_64bit);
