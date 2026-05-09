#include "dmg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

static uint16_t be16(uint8_t* p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static bool is_xz_compressed(uint8_t* data, size_t len) {
    if (len < 6) return false;
    return data[0] == 0xFD && data[1] == '7' && data[2] == 'z' &&
           data[3] == 'X' && data[4] == 'Z' && data[5] == 0x00;
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
    size_t in_pos = 0, out_pos = 0;
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

static uint8_t* decompress_lzma_chunk(uint8_t* compressed, size_t compressed_len,
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

static uint8_t* decompress_zlib_chunk(uint8_t* compressed, size_t compressed_len,
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
    (void)compressed;
    (void)compressed_len;
    *out_len = 0;
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
                strncpy(part->name, value, sizeof(part->name) - 1);
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

    fseeko64(ctx->file, 0, SEEK_END);
    uint64_t file_size = (uint64_t)ftello64(ctx->file);

    if (file_size < 512) {
        dmg_close(ctx);
        return false;
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
            fprintf(stdout, "\nNote: HFS+ filesystem mounting not yet implemented.\n");
            fprintf(stdout, "To extract apps manually:\n");
            fprintf(stdout, "  1. Mount the DMG using hdiutil on macOS\n");
            fprintf(stdout, "  2. Copy the .app bundle to your Applications folder\n");
            fprintf(stdout, "  3. Or use 7z/hfsplus to extract on Linux\n\n");
            free(decompressed);
        } else {
            fprintf(stderr, "Decompression failed\n");
        }
    } else {
        fprintf(stdout, "\nNote: This DMG uses an uncompressed data fork.\n");
        fprintf(stdout, "HFS+ filesystem support is needed for app extraction.\n");
    }

    dmg_close(ctx);
    return true;
}
