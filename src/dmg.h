#ifndef DMG_H
#define DMG_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t signature;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint64_t running_data_fork_offset;
    uint64_t data_fork_offset;
    uint64_t data_fork_length;
    uint64_t rsrc_fork_offset;
    uint64_t rsrc_fork_length;
    uint32_t segment_number;
    uint32_t segment_count;
    uint8_t segment_id[28];
    uint32_t data_checksum_type;
    uint32_t data_checksum_size;
    uint8_t data_checksum[32];
    uint64_t xml_offset;
    uint64_t xml_length;
    uint8_t reserved[120];
    uint32_t master_checksum_type;
    uint32_t master_checksum_size;
    uint8_t master_checksum[32];
    uint32_t image_variant;
    uint64_t sector_count;
    uint32_t rsrc_checksum_type;
    uint32_t rsrc_checksum_size;
    uint8_t rsrc_checksum[32];
} dmg_udif_footer;
#pragma pack(pop)

#define UDIF_SIGNATURE 0x6B6F6C79

typedef struct {
    char name[64];
    uint64_t offset;
    uint64_t length;
    uint32_t type;
} dmg_partition;

typedef struct {
    FILE* file;
    char path[512];
    dmg_udif_footer footer;
    dmg_partition* partitions;
    int partition_count;
    uint8_t* xml_plist;
    size_t xml_length;
} dmg_context;

bool dmg_open(const char* path, dmg_context** ctx);
void dmg_close(dmg_context* ctx);
bool dmg_print_info(dmg_context* ctx);
bool dmg_install_app(const char* dmg_path, const char* install_dir);

#endif
