#include "hfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#define fseeko64 fseeko
#define ftello64 ftello
#elif defined(_WIN32)
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#define mkdir(path, mode) _mkdir(path)
static int fseeko64(FILE* f, int64_t offset, int whence) { return _fseeki64(f, offset, whence); }
static int64_t ftello64(FILE* f) { return _ftelli64(f); }
#endif

static char g_error_msg[512] = {0};

const char* hfs_get_error(void) {
    return g_error_msg;
}

static void hfs_set_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_error_msg, sizeof(g_error_msg), fmt, args);
    va_end(args);
}

static uint32_t be32(uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t be16(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static uint64_t be64(uint8_t* p) {
    return ((uint64_t)be32(p) << 32) | (uint64_t)be32(p + 4);
}

static int hfs_strcmp_ignore_case(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = tolower((unsigned char)*s1);
        char c2 = tolower((unsigned char)*s2);
        if (c1 != c2) return (int)(unsigned char)c1 - (int)(unsigned char)c2;
        s1++;
        s2++;
    }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

static int hfs_ends_with(const char* str, const char* suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static uint32_t get_block_offset(hfs_context* ctx, uint32_t block_num) {
    return ctx->partition_offset + (uint64_t)block_num * ctx->block_size;
}

static bool read_volume_header(hfs_context* ctx) {
    uint8_t buf[512];

    fseeko64(ctx->image, (int64_t)ctx->partition_offset + 1024, SEEK_SET);
    if (fread(buf, 1, 512, ctx->image) != 512) {
        hfs_set_error("Failed to read volume header");
        return false;
    }

    uint32_t magic = be32(buf);
    if (magic == HFSPLUS_MAGIC) {
        memcpy(&ctx->vh, buf, sizeof(HFSPlusVolumeHeader));
        ctx->block_size = be32(buf + 0x10);
        ctx->total_blocks = be32(buf + 0x14);
        return true;
    }

    uint16_t hfs_magic = be16(buf);
    if (hfs_magic == HFS_MAGIC) {
        ctx->block_size = be16(buf + 0x10);
        ctx->total_blocks = be16(buf + 0x14);
        ctx->vh.signature = HFS_MAGIC;
        return true;
    }

    hfs_set_error("Invalid volume header magic: 0x%08x", magic);
    return false;
}

static bool read_catalog_btree(hfs_context* ctx) {
    uint32_t catalog_start = be32((uint8_t*)&ctx->vh + 0xD8);
    uint32_t catalog_blocks = be32((uint8_t*)&ctx->vh + 0xDC);
    uint64_t catalog_offset = get_block_offset(ctx, catalog_start);
    size_t catalog_size = (size_t)catalog_blocks * ctx->block_size;

    ctx->catalog_buffer = (uint8_t*)malloc(catalog_size);
    if (!ctx->catalog_buffer) {
        hfs_set_error("Failed to allocate catalog buffer");
        return false;
    }

    fseeko64(ctx->image, (off_t)catalog_offset, SEEK_SET);
    if (fread(ctx->catalog_buffer, 1, catalog_size, ctx->image) != catalog_size) {
        hfs_set_error("Failed to read catalog file");
        free(ctx->catalog_buffer);
        ctx->catalog_buffer = NULL;
        return false;
    }

    ctx->catalog_size = catalog_size;

    if (catalog_size < sizeof(HFSPlusBTreeHeader)) {
        hfs_set_error("Catalog too small");
        return false;
    }

    memcpy(&ctx->catalog_tree, ctx->catalog_buffer + 0x18, sizeof(HFSPlusBTreeHeader));
    ctx->catalog_tree.nodeSize = be16(ctx->catalog_buffer + 0x18);
    ctx->catalog_tree.rootNode = be32(ctx->catalog_buffer + 0x24);
    ctx->catalog_tree.firstLeafNode = be32(ctx->catalog_buffer + 0x2C);
    ctx->catalog_tree.maxKeyLength = be16(ctx->catalog_buffer + 0x3E);

    return true;
}

static uint8_t* get_node_ptr(hfs_context* ctx, uint32_t node_num) {
    if (ctx->catalog_buffer == NULL) return NULL;
    uint32_t node_offset = node_num * ctx->catalog_tree.nodeSize;
    if (node_offset + (uint32_t)ctx->catalog_tree.nodeSize > ctx->catalog_size) return NULL;
    return ctx->catalog_buffer + node_offset;
}

static bool is_leaf_node(uint8_t* node) {
    return (node[0] & 0x01) != 0;
}

static uint16_t get_node_num_records(uint8_t* node) {
    return be16(node + 0x0E);
}

static uint32_t get_node_next_sibling(uint8_t* node) {
    return be32(node + 0x0A);
}

static uint32_t get_node_parent(uint8_t* node) {
    return be32(node + 0x04);
}

static bool parse_hfsplus_name(const uint8_t* p, int len, char* out_name, int* out_len) {
    if (len < 2) return false;

    int name_len = p[0];
    if (name_len > len - 1 || name_len > 255) return false;

    int j = 0;
    for (int i = 1; i <= name_len && j < 255; i++) {
        uint16_t c = be16(p + i * 2);
        if (c < 0x80) {
            out_name[j++] = (char)c;
        } else if (c < 0x800) {
            if (j < 254) out_name[j++] = (char)(0xC0 | (c >> 6));
            if (j < 255) out_name[j++] = (char)(0x80 | (c & 0x3F));
        } else {
            if (j < 253) out_name[j++] = (char)(0xE0 | (c >> 12));
            if (j < 254) out_name[j++] = (char)(0x80 | ((c >> 6) & 0x3F));
            if (j < 255) out_name[j++] = (char)(0x80 | (c & 0x3F));
        }
    }
    out_name[j] = '\0';
    *out_len = j;
    return true;
}

static bool parse_hfs_name(const uint8_t* p, int len, char* out_name, int* out_len) {
    if (len < 1) return false;
    int name_len = p[0];
    if (name_len > len - 1) name_len = len - 1;
    if (name_len > 255) name_len = 255;
    memcpy(out_name, p + 1, name_len);
    out_name[name_len] = '\0';
    *out_len = name_len;
    return true;
}

static bool search_catalog(hfs_context* ctx, uint32_t node_num, uint32_t parent_id, const char* target_name, hfs_file_info* result, int depth) {
    if (depth > 16) return false;

    uint8_t* node = get_node_ptr(ctx, node_num);
    if (!node) return false;

    uint16_t num_records = get_node_num_records(node);
    uint16_t record_offsets = be16(node + ctx->catalog_tree.nodeSize - 2);

    for (uint16_t i = 0; i < num_records; i++) {
        uint16_t rec_offset = be16(node + record_offsets - i * 2);
        uint8_t* record = node + rec_offset;
        uint16_t key_len = 0;

        if (is_leaf_node(node)) {
            uint32_t record_type = be32(record);

            if (record_type == kHFSFileRecord || record_type == kHFSFolderRecord) {
                key_len = be16(record);
                uint32_t cnid = be32(record + key_len + 4);

                char name[256] = {0};
                int name_len = 0;

                if (key_len >= 5) {
                    uint8_t name_len_byte = record[5];
                    if (record_type == kHFSFileRecord && key_len >= 5 + 2 * name_len_byte) {
                        parse_hfsplus_name(record + 6, key_len - 5, name, &name_len);
                    } else if (record_type == kHFSFolderRecord && key_len >= 5 + 2 * name_len_byte) {
                        parse_hfsplus_name(record + 6, key_len - 5, name, &name_len);
                    }
                }

                uint32_t record_parent_id = be32(record + key_len + 8);

                if (record_type == kHFSFolderRecord && record_parent_id == parent_id) {
                    if (hfs_strcmp_ignore_case(name, target_name) == 0) {
                        result->cnid = cnid;
                        result->parent_cnid = record_parent_id;
                        strncpy(result->name, name, sizeof(result->name) - 1);
                        result->name[sizeof(result->name) - 1] = '\0';
                        result->is_directory = 1;
                        result->size = 0;
                        return true;
                    }

                    if (search_catalog(ctx, ctx->catalog_tree.rootNode, cnid, target_name, result, depth + 1)) {
                        return true;
                    }
                }

                if (record_type == kHFSFileRecord && record_parent_id == parent_id) {
                    if (hfs_strcmp_ignore_case(name, target_name) == 0) {
                        result->cnid = cnid;
                        result->parent_cnid = record_parent_id;
                        strncpy(result->name, name, sizeof(result->name) - 1);
                        result->name[sizeof(result->name) - 1] = '\0';
                        result->is_directory = 0;

                        memcpy(&result->data_extents, record + key_len + 12, sizeof(HFSPlusExtentRecord));
                        result->size = be64((uint8_t*)&result->data_extents + 0x18);

                        return true;
                    }
                }
            }
        } else {
            uint32_t child_node = be32(record + key_len + 4);
            if (search_catalog(ctx, child_node, parent_id, target_name, result, depth + 1)) {
                return true;
            }
        }
    }

    if (!is_leaf_node(node)) {
        uint32_t next_node = get_node_next_sibling(node);
        if (next_node != 0 && next_node != node_num) {
            if (search_catalog(ctx, next_node, parent_id, target_name, result, depth + 1)) {
                return true;
            }
        }
    }

    return false;
}

static bool traverse_directory(hfs_context* ctx, uint32_t node_num, uint32_t parent_id, int depth) {
    if (depth > 16) return false;

    uint8_t* node = get_node_ptr(ctx, node_num);
    if (!node) return false;

    uint16_t num_records = get_node_num_records(node);
    uint16_t record_offsets = be16(node + ctx->catalog_tree.nodeSize - 2);

    for (uint16_t i = 0; i < num_records; i++) {
        uint16_t rec_offset = be16(node + record_offsets - i * 2);
        uint8_t* record = node + rec_offset;
        uint16_t key_len = 0;

        if (is_leaf_node(node)) {
            uint32_t record_type = be32(record);

            if (record_type == kHFSFileRecord || record_type == kHFSFolderRecord) {
                key_len = be16(record);
                uint32_t cnid = be32(record + key_len + 4);
                uint32_t record_parent_id = be32(record + key_len + 8);

                if (record_parent_id != parent_id) continue;

                char name[256] = {0};
                int name_len = 0;

                if (record_type == kHFSFileRecord) {
                    HFSPlusFileRecord file_rec;
                    memcpy(&file_rec, record + key_len + 4, sizeof(HFSPlusFileRecord));
                    cnid = be32((uint8_t*)&file_rec.identifier);

                    if (key_len >= 5) {
                        uint8_t nl = record[5];
                        if (key_len >= 5 + 2 * nl) {
                            parse_hfsplus_name(record + 6, key_len - 5, name, &name_len);
                        }
                    }

                    printf("  %s%s (ID: %u, Size: %u bytes)\n",
                           name, hfs_ends_with(name, ".app") ? " [APP]" : "", cnid,
                           be32((uint8_t*)&file_rec + 0x30));
                } else {
                    HFSPlusFolderRecord folder_rec;
                    memcpy(&folder_rec, record + key_len + 4, sizeof(HFSPlusFolderRecord));
                    cnid = be32((uint8_t*)&folder_rec.folderID);

                    if (key_len >= 5) {
                        uint8_t nl = record[5];
                        if (key_len >= 5 + 2 * nl) {
                            parse_hfsplus_name(record + 6, key_len - 5, name, &name_len);
                        }
                    }

                    printf("  [DIR] %s/ (ID: %u)\n", name, cnid);
                }
            }
        } else {
            uint32_t child_node = be32(record + key_len + 4);
            traverse_directory(ctx, child_node, parent_id, depth + 1);
        }
    }

    return true;
}

static uint8_t* read_extent_data(hfs_context* ctx, HFSPlusExtentRecord* extents, uint64_t offset, uint64_t size, size_t* out_read) {
    uint8_t* buffer = (uint8_t*)malloc((size_t)size);
    if (!buffer) return NULL;

    size_t remaining = (size_t)size;
    size_t buf_pos = 0;
    uint64_t current_offset = 0;

    for (int i = 0; i < 8 && remaining > 0; i++) {
        uint32_t start_block = be32((uint8_t*)extents + i * 8);
        uint32_t block_count = be32((uint8_t*)extents + i * 8 + 4);

        if (start_block == 0 || block_count == 0) break;

        uint64_t extent_size = (uint64_t)block_count * ctx->block_size;
        uint64_t extent_start = get_block_offset(ctx, start_block);

        if (offset >= current_offset && offset < current_offset + extent_size) {
            uint64_t skip = offset - current_offset;
            uint64_t read_start = extent_start + skip;
            uint64_t read_size = extent_size - skip;

            if (read_size > remaining) read_size = remaining;

            fseeko64(ctx->image, (int64_t)read_start, SEEK_SET);
            size_t r = fread(buffer + buf_pos, 1, (size_t)read_size, ctx->image);
            if (r != (size_t)read_size) {
                free(buffer);
                return NULL;
            }

            buf_pos += r;
            remaining -= r;
        }

        current_offset += extent_size;
    }

    *out_read = buf_pos;
    return buffer;
}

static bool extract_to_directory(hfs_context* ctx, uint32_t dir_id, const char* base_path, int depth) {
    if (depth > 32) return false;

    uint8_t* root_node = get_node_ptr(ctx, ctx->catalog_tree.rootNode);
    if (!root_node) return false;

    uint16_t num_records = get_node_num_records(root_node);
    uint16_t record_offsets = be16(root_node + ctx->catalog_tree.nodeSize - 2);

    char dir_path[1024] = {0};
    if (depth > 0) {
        snprintf(dir_path, sizeof(dir_path), "%s", base_path);
    }

    for (uint16_t i = 0; i < num_records; i++) {
        uint16_t rec_offset = be16(root_node + record_offsets - i * 2);
        uint8_t* record = root_node + rec_offset;

        if (!is_leaf_node(root_node)) {
            continue;
        }

        uint32_t record_type = be32(record);
        if (record_type != kHFSFileRecord && record_type != kHFSFolderRecord) continue;

        uint16_t key_len = be16(record);
        uint32_t cnid = be32(record + key_len + 4);
        uint32_t record_parent_id = be32(record + key_len + 8);

        if (record_parent_id != dir_id) continue;

        char name[256] = {0};
        int name_len = 0;

        if (key_len >= 5) {
            uint8_t nl = record[5];
            if (key_len >= 5 + 2 * nl) {
                parse_hfsplus_name(record + 6, key_len - 5, name, &name_len);
            }
        }

        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);

        if (record_type == kHFSFolderRecord) {
            mkdir(full_path, 0755);
            extract_to_directory(ctx, cnid, full_path, depth + 1);
        } else {
            HFSPlusFileRecord file_rec;
            memcpy(&file_rec, record + key_len + 4, sizeof(HFSPlusFileRecord));

            uint64_t file_size = be64((uint8_t*)&file_rec + 0x30);
            HFSPlusExtentRecord data_extents;
            memcpy(&data_extents, (uint8_t*)&file_rec + 0x38, sizeof(HFSPlusExtentRecord));

            if (file_size > 0) {
                size_t bytes_read = 0;
                uint8_t* data = read_extent_data(ctx, &data_extents, 0, file_size, &bytes_read);
                if (data && bytes_read > 0) {
                    FILE* out = fopen(full_path, "wb");
                    if (out) {
                        fwrite(data, 1, bytes_read, out);
                        fclose(out);
                    }
                    free(data);
                }
            }
        }
    }

    return true;
}

bool hfs_mount(uint8_t* image_data, size_t image_size, hfs_context** ctx_out) {
    (void)image_data;

    hfs_context* ctx = (hfs_context*)calloc(1, sizeof(hfs_context));
    if (!ctx) return false;

    ctx->image = NULL;
    ctx->catalog_buffer = NULL;
    ctx->cache = NULL;
    ctx->cache_count = 0;

    *ctx_out = ctx;
    return true;
}

bool hfs_mount_from_file(FILE* image_file, uint64_t partition_offset, uint32_t block_size, hfs_context** ctx_out) {
    hfs_context* ctx = (hfs_context*)calloc(1, sizeof(hfs_context));
    if (!ctx) return false;

    ctx->image = image_file;
    ctx->partition_offset = partition_offset;
    ctx->block_size = block_size;
    ctx->catalog_buffer = NULL;
    ctx->cache = NULL;
    ctx->cache_count = 0;

    if (!read_volume_header(ctx)) {
        free(ctx);
        return false;
    }

    if (!read_catalog_btree(ctx)) {
        free(ctx);
        return false;
    }

    *ctx_out = ctx;
    return true;
}

void hfs_unmount(hfs_context* ctx) {
    if (!ctx) return;
    if (ctx->catalog_buffer) free(ctx->catalog_buffer);
    if (ctx->cache) free(ctx->cache);
    free(ctx);
}

bool hfs_list_directory(hfs_context* ctx, uint32_t dir_id) {
    if (!ctx || !ctx->catalog_buffer) return false;

    printf("\nContents of directory (ID: %u):\n", dir_id);
    printf("----------------------------------------\n");

    return traverse_directory(ctx, ctx->catalog_tree.rootNode, dir_id, 0);
}

bool hfs_find_app(hfs_context* ctx, hfs_file_info* result) {
    if (!ctx || !ctx->catalog_buffer || !result) return false;

    memset(result, 0, sizeof(hfs_file_info));

    return search_catalog(ctx, ctx->catalog_tree.rootNode, kHFSRootFolderID, ".app", result, 0);
}

bool hfs_extract_file(hfs_context* ctx, hfs_file_info* file, const char* output_path) {
    if (!ctx || !file || !output_path) return false;

    if (file->is_directory) {
        mkdir(output_path, 0755);
        return extract_to_directory(ctx, file->cnid, output_path, 0);
    }

    FILE* out = fopen(output_path, "wb");
    if (!out) {
        hfs_set_error("Failed to create output file: %s", output_path);
        return false;
    }

    HFSPlusExtentRecord extents;
    memcpy(&extents, &file->data_extents, sizeof(HFSPlusExtentRecord));

    uint64_t remaining = file->size;
    uint64_t offset = 0;

    while (remaining > 0) {
        size_t chunk_size = remaining > 1024 * 1024 ? 1024 * 1024 : (size_t)remaining;
        size_t bytes_read = 0;

        uint8_t* data = read_extent_data(ctx, &extents, offset, chunk_size, &bytes_read);
        if (!data || bytes_read == 0) break;

        fwrite(data, 1, bytes_read, out);
        free(data);

        offset += bytes_read;
        remaining -= bytes_read;
    }

    fclose(out);
    return true;
}

bool hfs_extract_app_bundle(hfs_context* ctx, const char* app_name, const char* output_dir) {
    if (!ctx || !app_name || !output_dir) return false;

    hfs_file_info result;
    memset(&result, 0, sizeof(result));

    if (!search_catalog(ctx, ctx->catalog_tree.rootNode, kHFSRootFolderID, app_name, &result, 0)) {
        hfs_set_error("Could not find app: %s", app_name);
        return false;
    }

    printf("Found %s (ID: %u, Directory: %s)\n", result.name, result.cnid, result.is_directory ? "Yes" : "No");

    char app_bundle_path[1024];
    snprintf(app_bundle_path, sizeof(app_bundle_path), "%s/%s", output_dir, app_name);

    mkdir(output_dir, 0755);
    mkdir(app_bundle_path, 0755);

    printf("Extracting to: %s\n", app_bundle_path);
    extract_to_directory(ctx, result.cnid, app_bundle_path, 0);

    return true;
}
