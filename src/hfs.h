#ifndef HFS_H
#define HFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#pragma pack(push, 1)

#define HFS_MAGIC 0x4244
#define HFSPLUS_MAGIC 0x482B
#define HFSPLUS_VERSION 4

#define kHFSRootFolderID 2
#define kHFSCatalogFileID 4

#define kHFSFolderRecord 1
#define kHFSFileRecord 2
#define kHFSFolderThreadRecord 3
#define kHFSFileThreadRecord 4

typedef struct {
    uint16_t signature;
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t reserved;
} HFSMasterDirectoryBlock;

typedef struct {
    uint32_t signature;
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t reserved;
    uint32_t createDate;
    uint32_t modifyDate;
    uint32_t backupDate;
    uint32_t checkedDate;
    uint32_t fileCount;
    uint32_t folderCount;
    uint32_t blockSize;
    uint32_t totalBlocks;
    uint32_t freeBlocks;
    uint32_t nextAllocation;
    uint32_t rsrcClumpSize;
    uint32_t dataClumpSize;
    uint32_t nextCatalogID;
    uint32_t writeCount;
    uint64_t encodingsBitmap;
    uint8_t finderInfo[32];
    uint8_t volumeUUID[16];
} HFSPlusVolumeHeader;

typedef struct {
    uint32_t startBlock;
    uint32_t blockCount;
} HFSPlusExtentDescriptor;

typedef struct {
    HFSPlusExtentDescriptor extents[8];
} HFSPlusExtentRecord;

typedef struct {
    uint32_t recordType;
    uint16_t flags;
    uint32_t valence;
    uint32_t folderID;
    uint32_t createDate;
    uint32_t contentModDate;
    uint32_t attributeModDate;
    uint32_t accessDate;
    uint32_t backupDate;
    uint32_t permissionsID;
    uint32_t userInfo[4];
    uint32_t adminInfo[4];
    uint32_t windowBounds[4];
    uint32_t textEncoding;
    uint32_t folderCount;
} HFSPlusFolderRecord;

typedef struct {
    uint32_t recordType;
    uint16_t flags;
    uint32_t reserved1;
    uint32_t createDate;
    uint32_t contentModDate;
    uint32_t attributeModDate;
    uint32_t accessDate;
    uint32_t backupDate;
    uint32_t permissionsID;
    uint32_t userInfo[4];
    uint32_t adminInfo[4];
    uint32_t mode;
    int32_t ownerID;
    int32_t groupID;
    uint32_t rsrcForkSize;
    HFSPlusExtentRecord dataFork;
    uint32_t contentType;
    uint32_t identifier;
    uint32_t reserved2;
    uint32_t textEncoding;
    uint32_t reserved3;
} HFSPlusFileRecord;

typedef struct {
    int32_t nodeSize;
    int16_t maxNodes;
    int16_t treeDepth;
    uint32_t rootNode;
    uint32_t leafRecords;
    uint32_t firstLeafNode;
    uint32_t lastLeafNode;
    int16_t freeNodes;
    int16_t reserved1;
    uint32_t clumpSize;
    uint8_t treeFlags;
    uint8_t reserved2;
    uint16_t maxKeyLength;
    uint32_t totalNodes;
    uint32_t freeNodes2;
} HFSPlusBTreeHeader;

#pragma pack(pop)

typedef struct {
    uint32_t node_num;
    uint16_t offset;
} hfs_node_ptr;

typedef struct {
    uint32_t cnid;
    uint32_t parent_cnid;
    char name[256];
    int is_directory;
    uint64_t size;
    HFSPlusExtentRecord data_extents;
    HFSPlusExtentRecord rsrc_extents;
} hfs_file_info;

typedef struct {
    FILE* image;
    HFSPlusVolumeHeader vh;
    uint32_t block_size;
    uint32_t total_blocks;
    uint64_t partition_offset;
    uint32_t catalog_file_id;
    uint32_t extent_file_id;
    HFSPlusBTreeHeader catalog_tree;
    uint8_t* catalog_buffer;
    size_t catalog_size;
    hfs_file_info* cache;
    int cache_count;
} hfs_context;

bool hfs_mount(uint8_t* image_data, size_t image_size, hfs_context** ctx_out);
bool hfs_mount_from_file(FILE* image_file, uint64_t partition_offset, uint32_t block_size, hfs_context** ctx_out);
void hfs_unmount(hfs_context* ctx);
bool hfs_list_directory(hfs_context* ctx, uint32_t dir_id);
bool hfs_find_app(hfs_context* ctx, hfs_file_info* result);
bool hfs_extract_file(hfs_context* ctx, hfs_file_info* file, const char* output_path);
bool hfs_extract_app_bundle(hfs_context* ctx, const char* app_name, const char* output_dir);
const char* hfs_get_error(void);

#endif
