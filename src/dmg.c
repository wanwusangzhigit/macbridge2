#include "dmg.h"
#include "hfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#ifdef __linux__
#include <unistd.h>
#include <lzma.h>
#include <zlib.h>
#define fseeko64 fseeko
#define ftello64 ftello
#elif defined(_WIN32)
#include <windows.h>
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#else
#include <unistd.h>
#include <zlib.h>
#endif

#define DMG_BUFFER_SIZE (64 * 1024)
#define BLOCK_TYPE_ZLIB 1
#define BLOCK_TYPE_LZMA 2
#define BLOCK_TYPE_UDCO 4
#define BLOCK_TYPE_UDIF 5
#define BLOCK_TYPE_COPY 6
#define BLOCK_TYPE_NULL 7
#define BLOCK_TYPE_END 8

#define HFSPLUS_MAGIC_BE 0x482B

typedef struct {
    uint32_t block_type;
    uint32_t block_number;
    uint32_t comment_size;
    uint64_t block_offset;
    uint64_t block_length;
    uint32_t checksum_type;
    uint32_t checksum_size;
    uint8_t checksum[32];
    uint8_t* data;
    uint32_t uncompressed_length;
} dmg_block_info;

static uint64_t be64(uint8_t* p) {
    return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) |
           ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
           ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
           ((uint64_t)p[6] << 8) | (uint64_t)p[7];
}

static uint32_t be32(uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint16_t __attribute__((unused)) be16(uint8_t* p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static bool is_xz_compressed(uint8_t* data, size_t len) {
    if (len < 6) return false;
    return data[0] == 0xFD && data[1] == '7' && data[2] == 'z' &&
           data[3] == 'X' && data[4] == 'Z' && data[5] == 0x00;
}

static bool is_raw_hfs_image(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    
    uint8_t buf[512];
    bool is_hfs = false;
    
    if (fread(buf, 1, 512, f) == 512) {
        uint16_t magic_be = (buf[0] << 8) | buf[1];
        if (magic_be == HFSPLUS_MAGIC_BE) {
            is_hfs = true;
        } else if (fseeko64(f, 36 * 512, SEEK_SET) == 0 && 
                   fread(buf, 1, 512, f) == 512) {
            magic_be = (buf[0] << 8) | buf[1];
            if (magic_be == HFSPLUS_MAGIC_BE) {
                is_hfs = true;
            }
        }
    }
    
    fclose(f);
    return is_hfs;
}

static uint8_t* decompress_xz_to_temp(FILE* dmg_file, uint64_t offset,
                                      uint64_t compressed_len,
                                      size_t* out_decompressed_len,
                                      uint64_t data_fork_length) {
    uint8_t* compressed_data = (uint8_t*)malloc((size_t)compressed_len);
    if (!compressed_data) return NULL;

    fseeko64(dmg_file, (off_t)offset, SEEK_SET);
    if (fread(compressed_data, 1, (size_t)compressed_len, dmg_file) != (size_t)compressed_len) {
        free(compressed_data);
        return NULL;
    }

    if (!is_xz_compressed(compressed_data, (size_t)compressed_len)) {
        free(compressed_data);
        return NULL;
    }

    size_t estimated_size = (data_fork_length > 0) ? (size_t)data_fork_length * 2 : 256 * 1024 * 1024;
    uint8_t* decompressed = (uint8_t*)malloc(estimated_size);
    if (!decompressed) {
        free(compressed_data);
        return NULL;
    }

#ifdef __linux__
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED);
    if (ret != LZMA_OK) {
        free(compressed_data);
        free(decompressed);
        return NULL;
    }

    lzma_action action = LZMA_RUN;
    size_t out_pos = 0;
    int streams_processed = 0;
    strm.next_in = compressed_data;
    strm.avail_in = (size_t)compressed_len;

    while (true) {
        if (strm.avail_in == 0 && action == LZMA_RUN) {
            streams_processed++;
            action = LZMA_FINISH;
        }

        strm.next_out = decompressed + out_pos;
        strm.avail_out = estimated_size - out_pos;

        ret = lzma_code(&strm, action);

        if (ret == LZMA_STREAM_END) {
            out_pos += (size_t)(estimated_size - out_pos - strm.avail_out);
            break;
        }

        if (ret != LZMA_OK) {
            if (out_pos > 0) break;
            free(compressed_data);
            free(decompressed);
            lzma_end(&strm);
            return NULL;
        }

        if (strm.avail_out == 0) {
            size_t new_size = estimated_size * 2;
            uint8_t* new_buf = (uint8_t*)realloc(decompressed, new_size);
            if (!new_buf) {
                free(compressed_data);
                free(decompressed);
                lzma_end(&strm);
                return NULL;
            }
            decompressed = new_buf;
            estimated_size = new_size;
        }
    }

    lzma_end(&strm);
    free(compressed_data);
    *out_decompressed_len = out_pos;
    return decompressed;

#elif defined(_WIN32)
    free(compressed_data);
    free(decompressed);
    *out_decompressed_len = 0;
    return NULL;
#else
    free(compressed_data);
    free(decompressed);
    *out_decompressed_len = 0;
    return NULL;
#endif
}

static __attribute__((unused)) uint8_t* decompress_lzma_chunk(uint8_t* compressed, size_t compressed_len,
                                       size_t* out_len, size_t expected_size) {
#ifdef __linux__
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret = lzma_alone_decoder(&strm, UINT64_MAX);

    if (ret != LZMA_OK) {
        return NULL;
    }

    uint8_t* decompressed = (uint8_t*)malloc(expected_size > 0 ? expected_size : compressed_len * 4);
    if (!decompressed) {
        lzma_end(&strm);
        return NULL;
    }

    strm.next_in = compressed;
    strm.avail_in = compressed_len;
    strm.next_out = decompressed;
    strm.avail_out = expected_size > 0 ? expected_size : compressed_len * 4;

    ret = lzma_code(&strm, LZMA_RUN);

    if (ret != LZMA_STREAM_END && expected_size > 0 && ret != LZMA_OK) {
        free(decompressed);
        lzma_end(&strm);
        return NULL;
    }

    *out_len = (size_t)(expected_size > 0 ? expected_size : compressed_len * 4 - strm.avail_out);
    lzma_end(&strm);
    return decompressed;
#else
    return NULL;
#endif
}

static __attribute__((unused)) uint8_t* decompress_zlib_chunk(uint8_t* compressed, size_t compressed_len,
                                       size_t* out_len, size_t expected_size) {
#ifdef __linux__
    z_stream strm;
    memset(&strm, 0, sizeof(strm));

    if (inflateInit(&strm) != Z_OK) {
        return NULL;
    }

    uint8_t* decompressed = (uint8_t*)malloc(expected_size > 0 ? expected_size : compressed_len * 4);
    if (!decompressed) {
        inflateEnd(&strm);
        return NULL;
    }

    strm.next_in = compressed;
    strm.avail_in = (uLong)compressed_len;
    strm.next_out = decompressed;
    strm.avail_out = (uLong)(expected_size > 0 ? expected_size : compressed_len * 4);

    int ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END && ret != Z_OK && expected_size > 0) {
        free(decompressed);
        inflateEnd(&strm);
        return NULL;
    }

    *out_len = (size_t)(expected_size > 0 ? expected_size : compressed_len * 4 - strm.avail_out);
    inflateEnd(&strm);
    return decompressed;
#else
    return NULL;
#endif
}

static bool dmg_parse_partitions(dmg_context* ctx, uint8_t* xml_data, size_t xml_len) {
    char* xml_str = (char*)xml_data;
    xml_str[xml_len - 1] = '\0';

    const char* blkx_start = strstr(xml_str, "<key>blkx</key>");
    if (!blkx_start) {
        const char* alt_start = strstr(xml_str, "<key>Blocks</key>");
        if (alt_start) blkx_start = alt_start;
    }
    if (!blkx_start) {
        const char* plist_start = strstr(xml_str, "<dict>");
        if (plist_start) blkx_start = plist_start;
    }
    if (!blkx_start) return false;

    const char* array_start = strstr(blkx_start, "<array>");
    if (!array_start) return false;

    int max_partitions = 16;
    ctx->partitions = (dmg_partition*)calloc(max_partitions, sizeof(dmg_partition));
    ctx->partition_count = 0;

    const char* dict_pos = array_start;
    while ((dict_pos = strstr(dict_pos, "<dict>")) && ctx->partition_count < max_partitions) {
        const char* dict_end = strstr(dict_pos, "</dict>");
        if (!dict_end) break;

        dmg_partition* part = &ctx->partitions[ctx->partition_count];
        memset(part, 0, sizeof(dmg_partition));

        const char* p = dict_pos;
        while ((p = strstr(p, "<key>")) && p < dict_end) {
            p += 5;
            const char* key_end = strstr(p, "</key>");
            if (!key_end || key_end > dict_end) break;

            size_t key_len = key_end - p;
            char key[64] = {0};
            if (key_len < sizeof(key)) strncpy(key, p, key_len);

            const char* value_start = strstr(key_end, ">");
            if (!value_start) break;
            value_start++;

            char value[256] = {0};
            const char* value_end;
            if (strncmp(value_start, "<string>", 8) == 0) {
                value_start += 8;
                value_end = strstr(value_start, "</string>");
                if (value_end && value_end < dict_end) {
                    size_t vlen = value_end - value_start;
                    if (vlen < sizeof(value)) strncpy(value, value_start, vlen);
                }
            } else if (strncmp(value_start, "<integer>", 9) == 0) {
                value_start += 9;
                value_end = strstr(value_start, "</integer>");
                if (value_end && value_end < dict_end) {
                    size_t vlen = value_end - value_start;
                    if (vlen < sizeof(value)) strncpy(value, value_start, vlen);
                }
            }

            if (strcmp(key, "Name") == 0) {
                size_t copy_len = strlen(value);
                if (copy_len >= sizeof(part->name)) copy_len = sizeof(part->name) - 1;
                memcpy(part->name, value, copy_len);
                part->name[copy_len] = '\0';
            } else if (strcmp(key, "BlockSize") == 0) {
            } else if (strcmp(key, "BlockOffset") == 0) {
                if (strlen(value) > 0) {
                    uint64_t offset = 0;
                    for (const char* c = value; *c; c++) {
                        if (*c >= '0' && *c <= '9') {
                            offset = offset * 10 + (*c - '0');
                        }
                    }
                    part->offset = offset;
                }
            } else if (strcmp(key, "BlockLength") == 0) {
                if (strlen(value) > 0) {
                    uint64_t len = 0;
                    for (const char* c = value; *c; c++) {
                        if (*c >= '0' && *c <= '9') {
                            len = len * 10 + (*c - '0');
                        }
                    }
                    part->length = len;
                }
            }

            p = key_end;
        }

        if (strlen(part->name) > 0 || part->offset > 0) {
            ctx->partition_count++;
        }

        dict_pos = dict_end + 7;
    }

    if (ctx->partition_count == 0) {
        dmg_partition* part = &ctx->partitions[0];
        strcpy(part->name, "disk image");
        part->offset = 0;
        part->length = ctx->footer.data_fork_length;
        ctx->partition_count = 1;
    }

    return ctx->partition_count > 0;
}

bool dmg_open(const char* path, dmg_context** ctx_out) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    dmg_context* ctx = (dmg_context*)calloc(1, sizeof(dmg_context));
    if (!ctx) {
        fclose(f);
        return false;
    }

    strncpy(ctx->path, path, sizeof(ctx->path) - 1);
    ctx->file = f;
    ctx->partitions = NULL;
    ctx->partition_count = 0;
    ctx->xml_plist = NULL;
    ctx->xml_length = 0;
    ctx->is_raw_hfs = false;
    ctx->partition_offset = 0;
    
    fseeko64(ctx->file, 0, SEEK_END);
    uint64_t file_size = (uint64_t)ftello64(ctx->file);
    
    if (file_size < 512) {
        dmg_close(ctx);
        return false;
    }
    
    if (is_raw_hfs_image(path)) {
        ctx->is_raw_hfs = true;
        ctx->partition_offset = 36 * 512;
        fprintf(stderr, "Detected raw HFS+ image at partition offset %lu\n", ctx->partition_offset);
        *ctx_out = ctx;
        return true;
    }
    
    uint8_t footer_buf[512];
    fseeko64(f, (off_t)(file_size - 512), SEEK_SET);
    if (fread(footer_buf, 1, 512, f) != 512) {
        dmg_close(ctx);
        return false;
    }

    uint32_t sig = be32(footer_buf);
    if (sig != UDIF_SIGNATURE) {
        fprintf(stderr, "Invalid UDIF signature: 0x%08x (expected 0x%08x)\n", sig, UDIF_SIGNATURE);
        dmg_close(ctx);
        return false;
    }

    memcpy(&ctx->footer, footer_buf, sizeof(dmg_udif_footer));

    uint64_t xml_offset = be64(footer_buf + 132);
    uint64_t xml_length = be64(footer_buf + 140);

    if (xml_offset == 0 || xml_length == 0 || xml_offset >= file_size) {
        xml_offset = (uint64_t)ctx->footer.data_fork_length;
        if (xml_offset > file_size - 1024) xml_offset = file_size - 1024;
        while (xml_offset > 0) {
            fseeko64(f, (off_t)xml_offset, SEEK_SET);
            uint8_t peek[8];
            if (fread(peek, 1, 8, f) == 8 && peek[0] == '<' &&
                (peek[1] == '?' || peek[1] == '!')) {
                break;
            }
            xml_offset -= 512;
        }
        if (xml_offset == 0) xml_offset = (uint64_t)ctx->footer.data_fork_length;

        uint64_t end_pos = file_size - 512;
        while (xml_offset < end_pos) {
            fseeko64(f, (off_t)xml_offset, SEEK_SET);
            uint8_t peek[8];
            if (fread(peek, 1, 8, f) == 8 && peek[0] == '<' &&
                (peek[1] == '?' || peek[1] == '!')) {
                break;
            }
            xml_offset += 512;
        }
        xml_length = end_pos - xml_offset;
    }

    ctx->xml_plist = (uint8_t*)malloc((size_t)xml_length + 1);
    if (!ctx->xml_plist) {
        dmg_close(ctx);
        return false;
    }

    fseeko64(f, (off_t)xml_offset, SEEK_SET);
    size_t read_len = fread(ctx->xml_plist, 1, (size_t)xml_length, f);
    ctx->xml_plist[read_len] = '\0';
    ctx->xml_length = read_len + 1;

    dmg_parse_partitions(ctx, ctx->xml_plist, ctx->xml_length);

    *ctx_out = ctx;
    return true;
}

void dmg_close(dmg_context* ctx) {
    if (!ctx) return;
    if (ctx->file) fclose(ctx->file);
    if (ctx->partitions) free(ctx->partitions);
    if (ctx->xml_plist) free(ctx->xml_plist);
    free(ctx);
}

bool dmg_print_info(dmg_context* ctx) {
    if (!ctx) return false;
    
    fprintf(stdout, "=== DMG Image Info ===\n");
    fprintf(stdout, "Path: %s\n", ctx->path);
    
    if (ctx->is_raw_hfs) {
        fprintf(stdout, "Type: Raw HFS+ disk image\n");
        fprintf(stdout, "Partition Offset: %lu (LBA 36)\n", ctx->partition_offset);
        fprintf(stdout, "HFS+ filesystem detected!\n");
        return true;
    }
    
    fprintf(stdout, "Version: %u\n", be32((uint8_t*)&ctx->footer.version));
    fprintf(stdout, "Header Size: %u\n", be32((uint8_t*)&ctx->footer.header_size));
    fprintf(stdout, "Total Sectors: %lu\n", (unsigned long)be64((uint8_t*)&ctx->footer.sector_count));
    fprintf(stdout, "Data Fork: offset=%lu, length=%lu\n",
            (unsigned long)be64((uint8_t*)&ctx->footer.data_fork_offset),
            (unsigned long)be64((uint8_t*)&ctx->footer.data_fork_length));
    fprintf(stdout, "XML Plist: offset=%lu, length=%lu\n\n",
            (unsigned long)be64((uint8_t*)&ctx->footer.xml_offset),
            (unsigned long)be64((uint8_t*)&ctx->footer.xml_length));

    if (ctx->partition_count > 0) {
        fprintf(stdout, "Partitions (%d):\n", ctx->partition_count);
        for (int i = ctx->partition_count - 1; i >= 0; i--) {
            dmg_partition* part = &ctx->partitions[i];
            fprintf(stdout, "  [%d] %s\n", i, part->name);
        }
    }

    return true;
}

bool dmg_install_app(const char* dmg_path, const char* install_dir) {
    fprintf(stdout, "Installing DMG: %s\n", dmg_path);
    fprintf(stdout, "Install directory: %s\n\n", install_dir ? install_dir : "(default)");

    dmg_context* ctx = NULL;
    if (!dmg_open(dmg_path, &ctx)) {
        fprintf(stderr, "Failed to open DMG file: %s\n", dmg_path);
        return false;
    }

    dmg_print_info(ctx);

    uint64_t data_fork_len = be64((uint8_t*)&ctx->footer.data_fork_length);

    if (data_fork_len > 0 && ctx->footer.data_fork_offset == 0) {
        fprintf(stdout, "\nDecompressing data fork...\n");

        size_t decompressed_len = 0;
        uint8_t* decompressed = decompress_xz_to_temp(ctx->file, 0, data_fork_len,
                                                       &decompressed_len,
                                                       data_fork_len);
        if (decompressed && decompressed_len > 0) {
            fprintf(stdout, "Decompressed: %zu bytes\n", decompressed_len);

            hfs_context* hfs_ctx = NULL;
            if (hfs_mount(decompressed, decompressed_len, &hfs_ctx)) {
                fprintf(stdout, "\nHFS+ filesystem detected!\n");

                if (install_dir == NULL) {
                    install_dir = "./Applications";
                }

                fprintf(stdout, "\nExtracting applications to: %s\n", install_dir);

                mkdir(install_dir, 0755);
                dmg_find_and_extract_app(ctx, "*.app", install_dir);

                hfs_unmount(hfs_ctx);
            } else {
                fprintf(stdout, "\nSearching for app bundles in decompressed data...\n");
                dmg_find_and_extract_app(ctx, "*.app", install_dir ? install_dir : "./Applications");
            }

            free(decompressed);
        } else {
            fprintf(stderr, "Decompression failed\n");
            dmg_close(ctx);
            return false;
        }
    } else {
        fprintf(stdout, "\nNote: This DMG uses an uncompressed data fork.\n");
        fprintf(stdout, "Attempting to mount HFS+ partition directly...\n");

        if (ctx->partition_count > 0) {
            for (int i = 0; i < ctx->partition_count; i++) {
                dmg_partition* part = &ctx->partitions[i];
                if (strstr(part->name, "Apple_HFS") != NULL ||
                    strstr(part->name, "disk image") != NULL) {
                    fprintf(stdout, "Found HFS+ partition at index %d\n", i);
                    dmg_extract_partition(ctx, i, install_dir ? install_dir : "./Applications");
                    break;
                }
            }
        }
    }

    dmg_close(ctx);
    return true;
}

static void ensure_directory(const char* path) {
    char dir[1024] = {0};
    const char* p = path;
    char* q = dir;

    while (*p) {
        *q++ = *p++;
        if (*p == '/' || *p == '\\' || *p == '\0') {
            *q = '\0';
            mkdir(dir, 0755);
        }
    }
}

bool dmg_list_files(dmg_context* ctx) {
    if (!ctx) return false;

    uint64_t data_fork_len = be64((uint8_t*)&ctx->footer.data_fork_length);

    if (data_fork_len == 0 || ctx->footer.data_fork_offset != 0) {
        fprintf(stderr, "Compressed data fork not available\n");
        return false;
    }

    size_t decompressed_len = 0;
    uint8_t* decompressed = decompress_xz_to_temp(ctx->file, 0, data_fork_len,
                                                   &decompressed_len,
                                                   data_fork_len);
    if (!decompressed || decompressed_len == 0) {
        fprintf(stderr, "Failed to decompress data fork\n");
        return false;
    }

    hfs_context* hfs_ctx = NULL;
    if (hfs_mount(decompressed, decompressed_len, &hfs_ctx)) {
        fprintf(stdout, "\nHFS+ filesystem detected! Listing root directory:\n");
        fprintf(stdout, "===========================================\n");
        hfs_list_directory(hfs_ctx, kHFSRootFolderID);
        hfs_unmount(hfs_ctx);
    } else {
        fprintf(stdout, "\nScanning decompressed data for .app bundles...\n");
        fprintf(stdout, "===========================================\n");

        char* data = (char*)decompressed;
        for (size_t i = 0; i < decompressed_len - 4; i++) {
            if (data[i] == '.' && data[i+1] == 'a' && data[i+2] == 'p' && data[i+3] == 'p') {
                char* start = data + i;
                char* end = start;
                while (end < data + decompressed_len && *end != '/' && *end != '\0') end++;
                if (end - start > 5) {
                    char name[256] = {0};
                    size_t len = end - start;
                    if (len > 255) len = 255;
                    memcpy(name, start, len);
                    fprintf(stdout, "Found: %s\n", name);
                    i = end - data;
                }
            }
        }
    }

    if (decompressed) free(decompressed);
    return true;
}

bool dmg_extract_partition(dmg_context* ctx, int partition_index, const char* output_dir) {
    if (!ctx || partition_index < 0 || partition_index >= ctx->partition_count) {
        fprintf(stderr, "Invalid partition index\n");
        return false;
    }

    dmg_partition* part = &ctx->partitions[partition_index];
    fprintf(stdout, "Extracting partition: %s\n", part->name);
    fprintf(stdout, "  Offset: %lu, Length: %lu\n", (unsigned long)part->offset, (unsigned long)part->length);

    uint64_t data_fork_len = be64((uint8_t*)&ctx->footer.data_fork_length);

    if (data_fork_len > 0 && ctx->footer.data_fork_offset == 0) {
        size_t decompressed_len = 0;
        uint8_t* decompressed = decompress_xz_to_temp(ctx->file, 0, data_fork_len,
                                                       &decompressed_len,
                                                       data_fork_len);
        if (decompressed && decompressed_len > 0) {
            fprintf(stdout, "Decompressed %zu bytes\n", decompressed_len);

            char output_path[1024];
            snprintf(output_path, sizeof(output_path), "%s/partition_%d.img",
                     output_dir ? output_dir : ".", partition_index);

            FILE* out = fopen(output_path, "wb");
            if (out) {
                fwrite(decompressed, 1, decompressed_len, out);
                fclose(out);
                fprintf(stdout, "Saved to: %s\n", output_path);
            }

            hfs_context* hfs_ctx = NULL;
            if (hfs_mount(decompressed, decompressed_len, &hfs_ctx)) {
                fprintf(stdout, "\nHFS+ detected in partition. Extracting files...\n");
                hfs_extract_app_bundle(hfs_ctx, "*.app", output_dir);
                hfs_unmount(hfs_ctx);
            }

            free(decompressed);
        }
    }

    return true;
}

bool dmg_find_and_extract_app(dmg_context* ctx, const char* app_pattern, const char* output_dir) {
    if (!ctx) return false;

    fprintf(stdout, "\nSearching for app bundles matching: %s\n", app_pattern);

    uint64_t data_fork_len = be64((uint8_t*)&ctx->footer.data_fork_length);

    if (data_fork_len == 0 || ctx->footer.data_fork_offset != 0) {
        fprintf(stderr, "No decompressed data available\n");
        return false;
    }

    size_t decompressed_len = 0;
    uint8_t* decompressed = decompress_xz_to_temp(ctx->file, 0, data_fork_len,
                                                   &decompressed_len,
                                                   data_fork_len);
    if (!decompressed || decompressed_len == 0) {
        fprintf(stderr, "Failed to decompress data fork\n");
        return false;
    }

    const char* target_dir = output_dir ? output_dir : "./Applications";
    ensure_directory(target_dir);

    hfs_context* hfs_ctx = NULL;
    if (hfs_mount(decompressed, decompressed_len, &hfs_ctx)) {
        fprintf(stdout, "Mounted HFS+ filesystem successfully\n");

        hfs_file_info result;
        if (hfs_find_app(hfs_ctx, &result)) {
            fprintf(stdout, "Found app: %s (CNID: %u)\n", result.name, result.cnid);

            char app_bundle_name[256] = {0};
            size_t copy_len = strlen(result.name);
            if (copy_len >= sizeof(app_bundle_name)) copy_len = sizeof(app_bundle_name) - 1;
            memcpy(app_bundle_name, result.name, copy_len);
            app_bundle_name[copy_len] = '\0';

            if (hfs_extract_app_bundle(hfs_ctx, app_bundle_name, target_dir)) {
                fprintf(stdout, "\nSuccessfully extracted: %s to %s\n", app_bundle_name, target_dir);
            }
        } else {
            fprintf(stdout, "\nNo .app bundle found at root level\n");
            fprintf(stdout, "Listing root directory contents:\n");
            hfs_list_directory(hfs_ctx, kHFSRootFolderID);
        }

        hfs_unmount(hfs_ctx);
    } else {
        fprintf(stdout, "HFS+ mount failed, scanning raw data...\n");

        char* data = (char*)decompressed;
        int found_count = 0;

        for (size_t i = 0; i < decompressed_len - 5 && found_count < 10; i++) {
            if (data[i] == '/' && data[i+1] == 'A' && data[i+2] == 'p' &&
                data[i+3] == 'p' && data[i+4] == 'l' && data[i+5] == 'i') {

                char* start = data + i;
                char* end = start + 1;
                while (end < data + decompressed_len && *end != '\0' &&
                       (isalnum((unsigned char)*end) || *end == '.' || *end == '_' || *end == '-' || *end == '/')) {
                    end++;
                }

                size_t path_len = end - start;
                if (path_len > 5 && path_len < 500) {
                    char path[512] = {0};
                    memcpy(path, start, path_len);

                    if (path_len > 4 && strcmp(path + path_len - 4, ".app") == 0) {
                        char app_name[256] = {0};
                        const char* last_slash = strrchr(path, '/');
                        if (last_slash) {
                            strncpy(app_name, last_slash + 1, sizeof(app_name) - 1);
                        }

                        fprintf(stdout, "Found app bundle: %s\n", app_name);

                        char out_path[1024];
                        snprintf(out_path, sizeof(out_path), "%s/%s", target_dir, app_name);
                        ensure_directory(out_path);

                        found_count++;
                    }
                }
            }
        }

        if (found_count == 0) {
            fprintf(stdout, "No .app bundles found in this DMG\n");
        } else {
            fprintf(stdout, "\nFound %d app bundle(s). Note: Full extraction requires HFS+ mount.\n", found_count);
            fprintf(stdout, "You can extract using: hdiutil attach %s (on macOS)\n", ctx->path);
        }
    }

    if (decompressed) free(decompressed);
    return true;
}

void dmg_set_verbose(bool verbose) {
    (void)verbose;
}

bool dmg_extract_from_raw_hfs(dmg_context* ctx, const char* output_dir) {
    if (!ctx || !ctx->is_raw_hfs) {
        fprintf(stderr, "Not a raw HFS+ image\n");
        return false;
    }
    
    const char* target_dir = output_dir ? output_dir : "./extracted";
    ensure_directory(target_dir);
    
    fprintf(stdout, "\nExtracting from raw HFS+ image: %s\n", ctx->path);
    fprintf(stdout, "Partition offset: %lu\n", (unsigned long)ctx->partition_offset);
    fprintf(stdout, "Output directory: %s\n\n", target_dir);
    
    fseeko64(ctx->file, 0, SEEK_END);
    uint64_t file_size = (uint64_t)ftello64(ctx->file);
    fprintf(stdout, "Image size: %lu bytes\n\n", (unsigned long)file_size);
    
    fprintf(stdout, "Scanning for Mach-O files and TRAE app bundles...\n");
    fprintf(stdout, "===========================================\n");
    
    FILE* strings_out = fopen("./extracted/trae_strings.txt", "w");
    if (!strings_out) {
        fprintf(stderr, "Failed to create strings output file\n");
    }
    
    uint8_t* buffer = (uint8_t*)malloc(8 * 1024 * 1024);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate buffer\n");
        if (strings_out) fclose(strings_out);
        return false;
    }
    
    int file_index = 0;
    int macho_found = 0;
    int strings_found = 0;
    char last_string[256] = {0};
    uint64_t last_string_offset = 0;
    
    for (uint64_t offset = 0; offset < file_size && file_index < 200; ) {
        uint64_t remaining = file_size - offset;
        size_t read_size = (remaining > 8 * 1024 * 1024) ? 8 * 1024 * 1024 : (size_t)remaining;
        
        fseeko64(ctx->file, (off_t)offset, SEEK_SET);
        size_t actual_read = fread(buffer, 1, read_size, ctx->file);
        
        if (actual_read < 64) break;
        
        for (size_t i = 0; i < actual_read - 4; i += 4) {
            uint32_t magic = *(uint32_t*)(buffer + i);
            
            if (magic == 0xFEEDFACF || magic == 0xCFFAEDFE ||
                magic == 0xFEEDFACE || magic == 0xCEFAEDFE) {
                
                uint32_t cputype = *(uint32_t*)(buffer + i + 4);
                uint32_t filetype = *(uint32_t*)(buffer + i + 12);
                
                size_t est_end = offset + i + 64 * 1024 * 1024;
                if (est_end > file_size) est_end = file_size;
                size_t extracted_size = (size_t)(est_end - (offset + i));
                
                const char* arch = "unknown";
                if ((cputype & 0xFF) == 0x00) arch = "arm64";
                else if ((cputype & 0xFF) == 0x01) arch = "x86";
                
                const char* type = "unknown";
                if (filetype == 2) type = "executable";
                else if (filetype == 6) type = "dylib";
                else if (filetype == 8) type = "bundle";
                
                char name[256];
                snprintf(name, sizeof(name), "macho_%04d_%s_%s.bin", 
                        file_index, arch, type);
                
                char out_path[512];
                snprintf(out_path, sizeof(out_path), "%s/%s", target_dir, name);
                
                FILE* out = fopen(out_path, "wb");
                if (out) {
                    fwrite(buffer + i, 1, extracted_size, out);
                    fclose(out);
                    
                    fprintf(stdout, "  Found Mach-O: %s (offset=%lu, size=%zu bytes, cputype=0x%X, filetype=0x%X)\n", 
                            name, (unsigned long)(offset + i), extracted_size, cputype, filetype);
                    
                    macho_found++;
                    file_index++;
                    i += 2 * 1024 * 1024;
                }
            }
            
            if (i < actual_read - 100) {
                if (buffer[i] == 'T' && buffer[i+1] == 'R' && buffer[i+2] == 'A' && buffer[i+3] == 'E') {
                    char strbuf[256] = {0};
                    size_t len = 0;
                    while (i + len < actual_read && len < 255 && buffer[i + len] >= 32 && buffer[i + len] < 127) {
                        strbuf[len] = buffer[i + len];
                        len++;
                    }
                    strbuf[len] = '\0';
                    
                    if (len > 10 && strcmp(strbuf, last_string) != 0 && 
                        (offset + i - last_string_offset > 1000 || last_string_offset == 0)) {
                        strncpy(last_string, strbuf, sizeof(last_string) - 1);
                        last_string_offset = offset + i;
                        
                        fprintf(stdout, "  Found: %s (offset=%lu)\n", strbuf, (unsigned long)(offset + i));
                        if (strings_out) {
                            fprintf(strings_out, "[%lu] %s\n", (unsigned long)(offset + i), strbuf);
                        }
                        strings_found++;
                    }
                }
            }
        }
        
        offset += (actual_read > 4 * 1024 * 1024) ? 4 * 1024 * 1024 : actual_read;
        
        if (file_index % 20 == 0 && file_index > 0) {
            fprintf(stdout, "\nProgress: Found %d Mach-O files, %d strings...\n", macho_found, strings_found);
        }
    }
    
    free(buffer);
    if (strings_out) fclose(strings_out);
    
    fprintf(stdout, "\n===========================================\n");
    fprintf(stdout, "Extraction complete!\n");
    fprintf(stdout, "Found %d Mach-O file(s) and %d interesting string(s)\n", macho_found, strings_found);
    fprintf(stdout, "Output directory: %s\n", target_dir);
    
    if (strings_found > 0) {
        fprintf(stdout, "\nSaved strings to: %s/trae_strings.txt\n", target_dir);
    }
    
    return true;
}
