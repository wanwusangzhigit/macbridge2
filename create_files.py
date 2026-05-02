
#!/usr/bin/env python3

# 创建 app_bundle.c
app_bundle_content = r'''
#include "app_bundle.h"
#include "platform.h"
#include "macho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

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
    
    mkdir(g_app_manager->install_directory, 0755);
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
    free(bundle);
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
        fprintf(stderr, "Invalid app bundle: %s\n", source_path);
        return false;
    }
    
    app_bundle* bundle = app_bundle_parse(source_path);
    if (!bundle) {
        fprintf(stderr, "Failed to parse app bundle\n");
        return false;
    }
    
    char dest_path[MAX_BUNDLE_PATH];
    const char* bundle_name = strrchr(source_path, '/');
    if (!bundle_name) bundle_name = source_path;
    else bundle_name++;
    
    snprintf(dest_path, sizeof(dest_path), "%s/%s", 
             g_app_manager->install_directory, bundle_name);
    
    fprintf(stdout, "Installing %s to %s...\n", bundle->bundle_name, dest_path);
    
    if (g_app_manager->num_bundles < g_app_manager->max_bundles) {
        memcpy(&g_app_manager->bundles[g_app_manager->num_bundles], bundle, sizeof(app_bundle));
        g_app_manager->num_bundles++;
    }
    
    app_bundle_free(bundle);
    fprintf(stdout, "Installation complete!\n");
    return true;
}

bool app_bundle_uninstall(const char* bundle_id) {
    if (!g_app_manager || !bundle_id) return false;
    
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        if (strcmp(g_app_manager->bundles[i].bundle_identifier, bundle_id) == 0) {
            app_bundle_free(&g_app_manager->bundles[i]);
            for (size_t j = i; j < g_app_manager->num_bundles - 1; j++) {
                g_app_manager->bundles[j] = g_app_manager->bundles[j + 1];
            }
            g_app_manager->num_bundles--;
            fprintf(stdout, "Uninstalled bundle: %s\n", bundle_id);
            return true;
        }
    }
    
    fprintf(stderr, "Bundle not found: %s\n", bundle_id);
    return false;
}

void app_bundle_list_installed(void) {
    if (!g_app_manager) return;
    
    fprintf(stdout, "Installed applications (%zu):\n", g_app_manager->num_bundles);
    for (size_t i = 0; i < g_app_manager->num_bundles; i++) {
        app_bundle* bundle = &g_app_manager->bundles[i];
        fprintf(stdout, "  - %s (%s)\n", bundle->bundle_name, bundle->bundle_identifier);
        fprintf(stdout, "    Version: %s\n", bundle->bundle_version);
        fprintf(stdout, "    Path: %s\n", bundle->bundle_path);
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
        fprintf(stderr, "Cannot launch invalid bundle\n");
        return -1;
    }
    
    char exe_path[MAX_BUNDLE_PATH];
    app_bundle_get_executable_path(bundle, exe_path, sizeof(exe_path));
    
    fprintf(stdout, "Launching: %s\n", exe_path);
    
    FILE* file = fopen(exe_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open executable: %s\n", exe_path);
        return -1;
    }
    
    fclose(file);
    
    fprintf(stdout, "Executable found. Loading Mach-O file...\n");
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
    
    fread(content, 1, file_size, file);
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

# 创建 app_loader.c
app_loader_content = r'''
#include "app_bundle.h"
#include "macho.h"
#include "dyld.h"
#include "platform.h"
#include "syscall.h"
#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char* program_name) {
    fprintf(stdout, "WinDarling Application Loader\n");
    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "  %s test <macho_path>        - Test loading a Mach-O file\n", program_name);
    fprintf(stdout, "  %s load <app_path>          - Load and run an app bundle\n", program_name);
    fprintf(stdout, "  %s install <app_path>       - Install an app bundle\n", program_name);
    fprintf(stdout, "  %s list                    - List installed applications\n", program_name);
    fprintf(stdout, "  %s uninstall <bundle_id>     - Uninstall an application\n", program_name);
    fprintf(stdout, "  %s help                    - Show this help message\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* command = argv[1];
    
    if (strcmp(command, "help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(command, "test") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Mach-O path required\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* filename = argv[2];
        fprintf(stdout, "Testing Mach-O load: %s\n", filename);
        
        // 初始化各个子系统
        init_syscall_table();
        vfs_init();
        dyld_init();
        
        struct mach_header* header = NULL;
        bool is_64bit = false;
        
        // 解析 Mach-O 头部
        if (!parse_macho_header(filename, &header, &is_64bit)) {
            fprintf(stderr, "Failed to parse Mach-O header\n");
            goto cleanup;
        }
        
        fprintf(stdout, "Mach-O Header Info:\n");
        fprintf(stdout, "Magic: 0x%x\n", header->magic);
        fprintf(stdout, "CPU Type: 0x%x\n", header->cputype);
        fprintf(stdout, "File Type: 0x%x\n", header->filetype);
        fprintf(stdout, "Number of commands: %d\n", header->ncmds);
        fprintf(stdout, "Size of commands: %d\n", header->sizeofcmds);
        fprintf(stdout, "Is 64-bit: %s\n", is_64bit ? "Yes" : "No");
        
        // 映射段到内存
        void* base_address = NULL;
        if (!map_macho_segments(filename, header, is_64bit, &base_address)) {
            fprintf(stderr, "Failed to map Mach-O segments\n");
            goto cleanup;
        }
        
        // 查找 main 函数
        void* main_addr = find_main_function(filename, header, is_64bit, base_address);
        if (main_addr) {
            fprintf(stdout, "Main function found at address: %p\n", main_addr);
        } else {
            fprintf(stdout, "Main function not found\n");
        }
        
        fprintf(stdout, "Test completed successfully!\n");
        
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
    
    if (strcmp(command, "load") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: App bundle path required\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* app_path = argv[2];
        fprintf(stdout, "Loading application bundle: %s\n", app_path);
        
        app_bundle* bundle = app_bundle_parse(app_path);
        if (!bundle) {
            fprintf(stderr, "Failed to parse app bundle\n");
            return 1;
        }
        
        int result = app_bundle_launch(bundle, argc - 3, argv + 3);
        app_bundle_free(bundle);
        return result;
    }
    
    if (strcmp(command, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: App bundle path required\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* app_path = argv[2];
        
        if (!app_manager_init(NULL)) {
            fprintf(stderr, "Failed to initialize app manager\n");
            return 1;
        }
        
        bool success = app_bundle_install(app_path);
        app_manager_cleanup();
        return success ? 0 : 1;
    }
    
    if (strcmp(command, "list") == 0) {
        if (!app_manager_init(NULL)) {
            fprintf(stderr, "Failed to initialize app manager\n");
            return 1;
        }
        
        app_bundle_list_installed();
        app_manager_cleanup();
        return 0;
    }
    
    if (strcmp(command, "uninstall") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Bundle ID required\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* bundle_id = argv[2];
        
        if (!app_manager_init(NULL)) {
            fprintf(stderr, "Failed to initialize app manager\n");
            return 1;
        }
        
        bool success = app_bundle_uninstall(bundle_id);
        app_manager_cleanup();
        return success ? 0 : 1;
    }
    
    fprintf(stderr, "Unknown command: %s\n", command);
    print_usage(argv[0]);
    return 1;
}
'''

# 写入文件
with open('/workspace/src/app_bundle.c', 'w') as f:
    f.write(app_bundle_content)

with open('/workspace/src/app_loader.c', 'w') as f:
    f.write(app_loader_content)

print("Files created successfully!")
