
#!/usr/bin/env python3

app_bundle_content = r'''
#include "app_bundle.h"
#include "platform.h"
#include "macho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

static app_manager* g_app_manager = NULL;

static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    char* result = (char*)malloc(strlen(str) + 1);
    if (result) strcpy(result, str);
    return result;
}

static void info_plist_entry_free(info_plist_entry* entry) {
    if (!entry) return;
    free(entry->key);
    free(entry->value);
    info_plist_entry_free((info_plist_entry*)entry->next);
    free(entry);
}

static bool create_directory_recursive(const char* path) {
    char tmp[1024];
    char* p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    return true;
}

static bool copy_file(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    if (!in) {
        fprintf(stderr, "Failed to open source file: %s\n", src);
        return false;
    }
    
    FILE* out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        fprintf(stderr, "Failed to create destination file: %s\n", dst);
        return false;
    }
    
    char buffer[8192];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, bytes, out) != bytes) {
            fprintf(stderr, "Failed to write to file: %s\n", dst);
            fclose(in);
            fclose(out);
            return false;
        }
    }
    
    fclose(in);
    fclose(out);
    return true;
}

static bool copy_directory_recursive(const char* src_dir, const char* dst_dir) {
    DIR* dir = opendir(src_dir);
    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", src_dir);
        return false;
    }
    
    if (!create_directory_recursive(dst_dir)) {
        fprintf(stderr, "Failed to create directory: %s\n", dst_dir);
        closedir(dir);
        return false;
    }
    
    struct dirent* entry;
    bool success = true;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char src_path[1024];
        char dst_path[1024];
        
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name);
        
        struct stat st;
        if (stat(src_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (!copy_directory_recursive(src_path, dst_path)) {
                    success = false;
                    break;
                }
            } else {
                if (!copy_file(src_path, dst_path)) {
                    success = false;
                    break;
                }
            }
        } else {
            fprintf(stderr, "Failed to stat: %s\n", src_path);
            success = false;
            break;
        }
    }
    
    closedir(dir);
    return success;
}

bool app_manager_init(const char* install_dir) {
    if (g_app_manager) return true;
    
    g_app_manager = (app_manager*)malloc(sizeof(app_manager));
    if (!g_app_manager) return false;
    
    memset(g_app_manager, 0, sizeof(app_manager));
    g_app_manager->max_bundles = 64;
    g_app_manager->bundles = (app_bundle*)malloc(g_app_manager->max_bundles * sizeof(app_bundle));
    
    if (!g_app_manager->bundles) {
        free(g_app_manager);
        g_app_manager = NULL;
        return false;
    }
    
    if (install_dir) {
        strncpy(g_app_manager->install_directory, install_dir, MAX_BUNDLE_PATH - 1);
    } else {
        strcpy(g_app_manager->install_directory, "./Applications");
    }
    
    create_directory_recursive(g_app_manager->install_directory);
    return true;
}

void app_manager_cleanup(void) {
    if (!g_app_manager) return;
    
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        app_bundle_free(&g_app_manager->bundles[i]);
    }
    
    free(g_app_manager->bundles);
    free(g_app_manager);
    g_app_manager = NULL;
}

app_bundle* app_bundle_parse(const char* bundle_path) {
    if (!bundle_path) return NULL;
    
    app_bundle* bundle = (app_bundle*)malloc(sizeof(app_bundle));
    if (!bundle) return NULL;
    
    memset(bundle, 0, sizeof(app_bundle));
    
    strncpy(bundle->bundle_path, bundle_path, MAX_BUNDLE_PATH - 1);
    
    char plist_path[MAX_BUNDLE_PATH];
    snprintf(plist_path, sizeof(plist_path), "%s/Contents/Info.plist", bundle_path);
    
    if (!parse_info_plist(plist_path, bundle)) {
        free(bundle);
        return NULL;
    }
    
    bundle->is_valid = true;
    return bundle;
}

void app_bundle_free(app_bundle* bundle) {
    if (!bundle) return;
    
    free(bundle->bundle_name);
    free(bundle->bundle_version);
    free(bundle->minimum_system_version);
    free(bundle->icon_file);
    info_plist_entry_free(bundle->info_plist_entries);
    memset(bundle, 0, sizeof(app_bundle));
}

const char* app_bundle_get_info_value(const app_bundle* bundle, const char* key) {
    if (!bundle || !key) return NULL;
    
    info_plist_entry* entry = bundle->info_plist_entries;
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = (info_plist_entry*)entry->next;
    }
    
    return NULL;
}

char* app_bundle_get_executable_path(const app_bundle* bundle, char* buffer, size_t buffer_size) {
    if (!bundle || !buffer) return NULL;
    
    snprintf(buffer, buffer_size, "%s/Contents/MacOS/%s", 
             bundle->bundle_path, bundle->executable_name);
    return buffer;
}

char* app_bundle_get_resource_path(const app_bundle* bundle, char* buffer, size_t buffer_size) {
    if (!bundle || !buffer) return NULL;
    
    snprintf(buffer, buffer_size, "%s/Contents/Resources", bundle->bundle_path);
    return buffer;
}

bool app_bundle_install(const char* source_path) {
    if (!g_app_manager || !source_path) return false;
    
    if (!is_valid_app_bundle(source_path)) {
        fprintf(stderr, "Error: Invalid app bundle at %s\n", source_path);
        return false;
    }
    
    app_bundle* bundle = app_bundle_parse(source_path);
    if (!bundle) {
        fprintf(stderr, "Error: Failed to parse app bundle\n");
        return false;
    }
    
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        if (strcmp(g_app_manager->bundles[i].bundle_identifier, bundle->bundle_identifier) == 0) {
            fprintf(stderr, "Error: App %s is already installed\n", bundle->bundle_name);
            app_bundle_free(bundle);
            free(bundle);
            return false;
        }
    }
    
    const char* bundle_name = strrchr(source_path, '/');
    if (!bundle_name) bundle_name = source_path;
    else bundle_name++;
    
    char dest_path[MAX_BUNDLE_PATH];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", 
             g_app_manager->install_directory, bundle_name);
    
    fprintf(stdout, "Installing %s...\n", bundle->bundle_name);
    fprintf(stdout, "Source: %s\n", source_path);
    fprintf(stdout, "Destination: %s\n", dest_path);
    
    if (!copy_directory_recursive(source_path, dest_path)) {
        fprintf(stderr, "Error: Failed to copy application files\n");
        app_bundle_free(bundle);
        free(bundle);
        return false;
    }
    
    app_bundle* installed_bundle = app_bundle_parse(dest_path);
    if (!installed_bundle) {
        fprintf(stderr, "Error: Failed to parse installed app bundle\n");
        app_bundle_free(bundle);
        free(bundle);
        return false;
    }
    
    if (g_app_manager->num_bundles < g_app_manager->max_bundles) {
        memcpy(&g_app_manager->bundles[g_app_manager->num_bundles], installed_bundle, sizeof(app_bundle));
        g_app_manager->num_bundles++;
    }
    
    app_bundle_free(bundle);
    free(bundle);
    
    fprintf(stdout, "Installation complete!\n");
    return true;
}

bool app_bundle_uninstall(const char* bundle_id) {
    if (!g_app_manager || !bundle_id) return false;
    
    size_t index = (size_t)-1;
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        if (strcmp(g_app_manager->bundles[i].bundle_identifier, bundle_id) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == (size_t)-1) {
        fprintf(stderr, "Error: Bundle not found: %s\n", bundle_id);
        return false;
    }
    
    char app_path[MAX_BUNDLE_PATH];
    strncpy(app_path, g_app_manager->bundles[index].bundle_path, MAX_BUNDLE_PATH - 1);
    
    char rm_cmd[1024];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf \"%s\"", app_path);
    
    fprintf(stdout, "Uninstalling %s...\n", g_app_manager->bundles[index].bundle_name);
    
    int ret = system(rm_cmd);
    if (ret != 0) {
        fprintf(stderr, "Warning: Failed to remove application files (exit code: %d)\n", ret);
    }
    
    app_bundle_free(&g_app_manager->bundles[index]);
    
    for (size_t j = index; j < g_app_manager->num_bundles - 1; j++) {
        g_app_manager->bundles[j] = g_app_manager->bundles[j + 1];
    }
    memset(&g_app_manager->bundles[g_app_manager->num_bundles - 1], 0, sizeof(app_bundle));
    g_app_manager->num_bundles--;
    
    fprintf(stdout, "Uninstalled successfully!\n");
    return true;
}

void app_bundle_list_installed(void) {
    if (!g_app_manager) {
        fprintf(stdout, "No applications installed.\n");
        return;
    }
    
    if (g_app_manager->num_bundles == 0) {
        fprintf(stdout, "No applications installed.\n");
        return;
    }
    
    fprintf(stdout, "Installed applications (%zu):\n\n", g_app_manager->num_bundles);
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        app_bundle* bundle = &g_app_manager->bundles[i];
        fprintf(stdout, "  [%zu] %s\n", i + 1, bundle->bundle_name ? bundle->bundle_name : "Unknown");
        fprintf(stdout, "      ID: %s\n", bundle->bundle_identifier);
        fprintf(stdout, "      Version: %s\n", bundle->bundle_version ? bundle->bundle_version : "N/A");
        fprintf(stdout, "      Path: %s\n", bundle->bundle_path);
        if (bundle->minimum_system_version) {
            fprintf(stdout, "      Requires: macOS %s+\n", bundle->minimum_system_version);
        }
        fprintf(stdout, "\n");
    }
}

app_bundle* app_bundle_find_by_id(const char* bundle_id) {
    if (!g_app_manager || !bundle_id) return NULL;
    
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        if (strcmp(g_app_manager->bundles[i].bundle_identifier, bundle_id) == 0) {
            return &g_app_manager->bundles[i];
        }
    }
    
    return NULL;
}

app_bundle* app_bundle_find_by_name(const char* bundle_name) {
    if (!g_app_manager || !bundle_name) return NULL;
    
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        if (g_app_manager->bundles[i].bundle_name && 
            strcmp(g_app_manager->bundles[i].bundle_name, bundle_name) == 0) {
            return &g_app_manager->bundles[i];
        }
    }
    
    return NULL;
}

int app_bundle_launch(const app_bundle* bundle, int argc, char* argv[]) {
    if (!bundle || !bundle->is_valid) {
        fprintf(stderr, "Error: Cannot launch invalid bundle\n");
        return -1;
    }
    
    char exe_path[MAX_BUNDLE_PATH];
    app_bundle_get_executable_path(bundle, exe_path, sizeof(exe_path));
    
    fprintf(stdout, "Launching: %s\n", bundle->bundle_name ? bundle->bundle_name : "Unknown");
    fprintf(stdout, "Executable: %s\n", exe_path);
    
    FILE* file = fopen(exe_path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Executable not found at %s\n", exe_path);
        return -1;
    }
    fclose(file);
    
    fprintf(stdout, "Executable found. Loading Mach-O file...\n");
    
    struct mach_header* header = NULL;
    bool is_64bit = false;
    
    if (!parse_macho_header(exe_path, &header, &is_64bit)) {
        fprintf(stderr, "Error: Failed to parse Mach-O header\n");
        return -1;
    }
    
    fprintf(stdout, "Mach-O Header:\n");
    fprintf(stdout, "  Magic: 0x%x\n", header->magic);
    fprintf(stdout, "  CPU Type: 0x%x\n", header->cputype);
    fprintf(stdout, "  File Type: 0x%x\n", header->filetype);
    fprintf(stdout, "  Number of commands: %d\n", header->ncmds);
    fprintf(stdout, "  64-bit: %s\n", is_64bit ? "Yes" : "No");
    
    void* base_address = NULL;
    if (!map_macho_segments(exe_path, header, is_64bit, &base_address)) {
        fprintf(stderr, "Error: Failed to map Mach-O segments\n");
        free(header);
        return -1;
    }
    
    void* main_addr = find_main_function(exe_path, header, is_64bit, base_address);
    if (main_addr) {
        fprintf(stdout, "Main function found at: %p\n", main_addr);
        fprintf(stdout, "\nReady to execute main function (execution not implemented in demo mode)\n");
    } else {
        fprintf(stdout, "Main function not found (this is normal for some Mach-O files)\n");
    }
    
    unmap_macho_segments(base_address, header, is_64bit);
    
    fprintf(stdout, "Launch completed!\n");
    return 0;
}

bool is_valid_app_bundle(const char* path) {
    if (!path) return false;
    
    char plist_path[MAX_BUNDLE_PATH];
    snprintf(plist_path, sizeof(plist_path), "%s/Contents/Info.plist", path);
    
    FILE* file = fopen(plist_path, "r");
    if (!file) return false;
    fclose(file);
    
    return true;
}

bool parse_info_plist(const char* plist_path, app_bundle* bundle) {
    if (!plist_path || !bundle) return false;
    
    FILE* file = fopen(plist_path, "r");
    if (!file) return false;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return false;
    }
    
    if (fread(content, 1, file_size, file) != (size_t)file_size) {
        fprintf(stderr, "Warning: Incomplete read of plist file\n");
    }
    content[file_size] = '\0';
    fclose(file);
    
    char buffer[256];
    
    if (extract_plist_key_value(content, "CFBundleExecutable", buffer, sizeof(buffer))) {
        strncpy(bundle->executable_name, buffer, MAX_EXECUTABLE_NAME - 1);
    }
    
    if (extract_plist_key_value(content, "CFBundleIdentifier", buffer, sizeof(buffer))) {
        strncpy(bundle->bundle_identifier, buffer, MAX_BUNDLE_ID - 1);
    }
    
    if (extract_plist_key_value(content, "CFBundleName", buffer, sizeof(buffer))) {
        bundle->bundle_name = strdup_safe(buffer);
    }
    
    if (extract_plist_key_value(content, "CFBundleShortVersionString", buffer, sizeof(buffer))) {
        bundle->bundle_version = strdup_safe(buffer);
    } else if (extract_plist_key_value(content, "CFBundleVersion", buffer, sizeof(buffer))) {
        bundle->bundle_version = strdup_safe(buffer);
    }
    
    if (extract_plist_key_value(content, "LSMinimumSystemVersion", buffer, sizeof(buffer))) {
        bundle->minimum_system_version = strdup_safe(buffer);
    }
    
    if (extract_plist_key_value(content, "CFBundleIconFile", buffer, sizeof(buffer))) {
        bundle->icon_file = strdup_safe(buffer);
    }
    
    free(content);
    return true;
}

bool extract_plist_key_value(const char* plist_content, const char* key, char* value, size_t value_size) {
    if (!plist_content || !key || !value) return false;
    
    char key_tag[256];
    snprintf(key_tag, sizeof(key_tag), "<key>%s</key>", key);
    
    const char* key_pos = strstr(plist_content, key_tag);
    if (!key_pos) return false;
    
    const char* value_start = strchr(key_pos + strlen(key_tag), '>');
    if (!value_start) return false;
    value_start++;
    
    const char* value_end = strchr(value_start, '<');
    if (!value_end) return false;
    
    size_t len = value_end - value_start;
    if (len >= value_size) len = value_size - 1;
    
    strncpy(value, value_start, len);
    value[len] = '\0';
    
    return true;
}
'''

with open('/workspace/src/app_bundle.c', 'w') as f:
    f.write(app_bundle_content)

print("Fixed memory management in app_bundle.c!")
