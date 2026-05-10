#include "app_bundle.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    free(entry);
}

bool extract_plist_key_value(const char* content, const char* key, char* value, size_t value_size) {
    if (!content || !key || !value) return false;
    
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "<key>%s</key>", key);
    
    char* key_pos = strstr(content, pattern);
    if (!key_pos) return false;
    
    char* value_start = strstr(key_pos, "<string>");
    if (!value_start) return false;
    
    value_start += 8;
    char* value_end = strstr(value_start, "</string>");
    if (!value_end) return false;
    
    size_t value_len = value_end - value_start;
    if (value_len >= value_size) value_len = value_size - 1;
    
    strncpy(value, value_start, value_len);
    value[value_len] = '\0';
    
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
    
    size_t bytes_read = fread(content, 1, file_size, file);
    if (bytes_read != file_size) {
        free(content);
        fclose(file);
        return false;
    }
    content[file_size] = '\0';
    fclose(file);
    
    char buffer[1024];
    
    if (extract_plist_key_value(content, "CFBundleExecutable", buffer, sizeof(buffer))) {
        strncpy(bundle->executable_name, buffer, MAX_EXECUTABLE_NAME - 1);
        bundle->executable_name[MAX_EXECUTABLE_NAME - 1] = '\0';
    }
    
    if (extract_plist_key_value(content, "CFBundleIdentifier", buffer, sizeof(buffer))) {
        strncpy(bundle->bundle_identifier, buffer, MAX_BUNDLE_ID - 1);
        bundle->bundle_identifier[MAX_BUNDLE_ID - 1] = '\0';
    }
    
    if (extract_plist_key_value(content, "CFBundleName", buffer, sizeof(buffer))) {
        bundle->bundle_name = strdup_safe(buffer);
    }
    
    if (extract_plist_key_value(content, "CFBundleVersion", buffer, sizeof(buffer))) {
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

bool is_valid_app_bundle(const char* path) {
    if (!path) return false;
    
    char plist_path[MAX_BUNDLE_PATH];
    snprintf(plist_path, sizeof(plist_path), "%s/Contents/Info.plist", path);
    
    FILE* file = fopen(plist_path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

static bool create_directory_recursive(const char* path) {
    char tmp[4096];
    char* p = NULL;
    size_t len;
    
    if (strlen(path) >= sizeof(tmp)) {
        fprintf(stderr, "Path too long: %s\n", path);
        return false;
    }
    
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    if (len > 0 && tmp[len - 1] == '/') {
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
        
        char src_path[4096];
        char dst_path[4096];
        
        if (snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name) >= (int)sizeof(src_path)) {
            fprintf(stderr, "Source path too long: %s/%s\n", src_dir, entry->d_name);
            success = false;
            break;
        }
        
        if (snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name) >= (int)sizeof(dst_path)) {
            fprintf(stderr, "Destination path too long: %s/%s\n", dst_dir, entry->d_name);
            success = false;
            break;
        }
        
        struct stat stat_buf;
        if (stat(src_path, &stat_buf) == 0) {
            if (S_ISDIR(stat_buf.st_mode)) {
                if (!copy_directory_recursive(src_path, dst_path)) {
                    success = false;
                    break;
                }
            } else {
                FILE* src_file = fopen(src_path, "rb");
                if (!src_file) {
                    fprintf(stderr, "Failed to open file: %s\n", src_path);
                    success = false;
                    break;
                }
                
                FILE* dst_file = fopen(dst_path, "wb");
                if (!dst_file) {
                    fprintf(stderr, "Failed to create file: %s\n", dst_path);
                    fclose(src_file);
                    success = false;
                    break;
                }
                
                char buffer[8192];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
                    fwrite(buffer, 1, bytes, dst_file);
                }
                
                fclose(src_file);
                fclose(dst_file);
            }
        }
    }
    
    closedir(dir);
    return success;
}

static bool remove_directory_recursive(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) {
        return false;
    }
    
    struct dirent* entry;
    bool success = true;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat stat_buf;
        if (stat(full_path, &stat_buf) == 0) {
            if (S_ISDIR(stat_buf.st_mode)) {
                if (!remove_directory_recursive(full_path)) {
                    success = false;
                }
            } else {
                if (unlink(full_path) != 0) {
                    fprintf(stderr, "Failed to delete file: %s\n", full_path);
                    success = false;
                }
            }
        }
    }
    
    closedir(dir);
    
    if (rmdir(path) != 0) {
        fprintf(stderr, "Failed to remove directory: %s\n", path);
        success = false;
    }
    
    return success;
}

bool app_manager_init(const char* install_dir) {
    if (g_app_manager) {
        return true;
    }
    
    g_app_manager = (app_manager*)malloc(sizeof(app_manager));
    if (!g_app_manager) {
        return false;
    }
    
    if (install_dir) {
        strncpy(g_app_manager->install_directory, install_dir, MAX_BUNDLE_PATH - 1);
    } else {
        snprintf(g_app_manager->install_directory, MAX_BUNDLE_PATH, "./Applications");
    }
    
    create_directory_recursive(g_app_manager->install_directory);
    
    g_app_manager->bundles = NULL;
    g_app_manager->num_bundles = 0;
    g_app_manager->max_bundles = 16;
    g_app_manager->bundles = (app_bundle*)malloc(sizeof(app_bundle) * g_app_manager->max_bundles);
    
    if (!g_app_manager->bundles) {
        free(g_app_manager);
        g_app_manager = NULL;
        return false;
    }
    
    return app_manager_load();
}

bool app_manager_load(void) {
    if (!g_app_manager) return false;
    
    char db_path[1024];
    snprintf(db_path, sizeof(db_path), "%s/.app_db", g_app_manager->install_directory);
    
    FILE* db_file = fopen(db_path, "rb");
    if (db_file) {
        int count;
        if (fread(&count, sizeof(int), 1, db_file) == 1) {
            for (int i = 0; i < count && i < g_app_manager->max_bundles; i++) {
                memset(&g_app_manager->bundles[i], 0, sizeof(app_bundle));
                
                size_t len;
                if (fread(&len, sizeof(size_t), 1, db_file) == 1 && len > 0 && len <= MAX_BUNDLE_ID) {
                    if (fread(g_app_manager->bundles[i].bundle_identifier, 1, len, db_file) != len) break;
                }
                
                if (fread(&len, sizeof(size_t), 1, db_file) == 1 && len > 0 && len <= MAX_BUNDLE_PATH) {
                    if (fread(g_app_manager->bundles[i].bundle_path, 1, len, db_file) != len) break;
                }
                
                if (fread(&len, sizeof(size_t), 1, db_file) == 1 && len > 0) {
                    char* str = (char*)malloc(len);
                    if (str) {
                        if (fread(str, 1, len, db_file) != len) {
                            free(str);
                            break;
                        }
                        g_app_manager->bundles[i].bundle_name = str;
                    }
                }
                
                if (fread(&len, sizeof(size_t), 1, db_file) == 1 && len > 0) {
                    char* str = (char*)malloc(len);
                    if (str) {
                        if (fread(str, 1, len, db_file) != len) {
                            free(str);
                            break;
                        }
                        g_app_manager->bundles[i].bundle_version = str;
                    }
                }
                
                if (fread(&len, sizeof(size_t), 1, db_file) == 1 && len > 0) {
                    char* str = (char*)malloc(len);
                    if (str) {
                        if (fread(str, 1, len, db_file) != len) {
                            free(str);
                            break;
                        }
                        g_app_manager->bundles[i].minimum_system_version = str;
                    }
                }
                
                if (fread(&len, sizeof(size_t), 1, db_file) == 1 && len > 0) {
                    char* str = (char*)malloc(len);
                    if (str) {
                        if (fread(str, 1, len, db_file) != len) {
                            free(str);
                            break;
                        }
                        g_app_manager->bundles[i].icon_file = str;
                    }
                }
                
                g_app_manager->bundles[i].is_valid = true;
                g_app_manager->bundles[i].info_plist_entries = NULL;
                g_app_manager->bundles[i].num_plist_entries = 0;
                
                g_app_manager->num_bundles++;
            }
        }
        fclose(db_file);
    }
    return true;
}

bool app_manager_save(void) {
    if (!g_app_manager) return false;
    
    char db_path[1024];
    snprintf(db_path, sizeof(db_path), "%s/.app_db", g_app_manager->install_directory);
    
    FILE* db_file = fopen(db_path, "wb");
    if (db_file) {
        fwrite(&g_app_manager->num_bundles, sizeof(int), 1, db_file);
        for (int i = 0; i < g_app_manager->num_bundles; i++) {
            size_t len = strlen(g_app_manager->bundles[i].bundle_identifier) + 1;
            fwrite(&len, sizeof(size_t), 1, db_file);
            fwrite(g_app_manager->bundles[i].bundle_identifier, 1, len, db_file);
            
            len = strlen(g_app_manager->bundles[i].bundle_path) + 1;
            fwrite(&len, sizeof(size_t), 1, db_file);
            fwrite(g_app_manager->bundles[i].bundle_path, 1, len, db_file);
            
            len = g_app_manager->bundles[i].bundle_name ? strlen(g_app_manager->bundles[i].bundle_name) + 1 : 0;
            fwrite(&len, sizeof(size_t), 1, db_file);
            if (len > 0) {
                fwrite(g_app_manager->bundles[i].bundle_name, sizeof(char), len, db_file);
            }
            
            len = g_app_manager->bundles[i].bundle_version ? strlen(g_app_manager->bundles[i].bundle_version) + 1 : 0;
            fwrite(&len, sizeof(size_t), 1, db_file);
            if (len > 0) {
                fwrite(g_app_manager->bundles[i].bundle_version, sizeof(char), len, db_file);
            }
            
            len = g_app_manager->bundles[i].minimum_system_version ? strlen(g_app_manager->bundles[i].minimum_system_version) + 1 : 0;
            fwrite(&len, sizeof(size_t), 1, db_file);
            if (len > 0) {
                fwrite(g_app_manager->bundles[i].minimum_system_version, sizeof(char), len, db_file);
            }
            
            len = g_app_manager->bundles[i].icon_file ? strlen(g_app_manager->bundles[i].icon_file) + 1 : 0;
            fwrite(&len, sizeof(size_t), 1, db_file);
            if (len > 0) {
                fwrite(g_app_manager->bundles[i].icon_file, sizeof(char), len, db_file);
            }
        }
        fclose(db_file);
        return true;
    }
    return false;
}

void app_manager_cleanup(void) {
    if (!g_app_manager) return;
    
    app_manager_save();
    
    for (int i = 0; i < g_app_manager->num_bundles; i++) {
        app_bundle_free_resources(&g_app_manager->bundles[i]);
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
    bundle->bundle_path[MAX_BUNDLE_PATH - 1] = '\0';
    
    char info_plist_path[MAX_BUNDLE_PATH];
    snprintf(info_plist_path, sizeof(info_plist_path), "%s/Contents/Info.plist", bundle_path);
    
    if (parse_info_plist(info_plist_path, bundle)) {
        bundle->is_valid = true;
    }
    
    return bundle;
}

void app_bundle_free_resources(app_bundle* bundle) {
    if (!bundle) return;
    free(bundle->bundle_name);
    free(bundle->bundle_version);
    free(bundle->minimum_system_version);
    free(bundle->icon_file);
    
    info_plist_entry* entry = bundle->info_plist_entries;
    while (entry) {
        info_plist_entry* next = (info_plist_entry*)entry->next;
        info_plist_entry_free(entry);
        entry = next;
    }
    
    bundle->bundle_name = NULL;
    bundle->bundle_version = NULL;
    bundle->minimum_system_version = NULL;
    bundle->icon_file = NULL;
    bundle->info_plist_entries = NULL;
    bundle->num_plist_entries = 0;
}

void app_bundle_free(app_bundle* bundle) {
    if (!bundle) return;
    app_bundle_free_resources(bundle);
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
    
    snprintf(buffer, buffer_size, "%s/Contents/MacOS/%s", bundle->bundle_path, bundle->executable_name);
    return buffer;
}

char* app_bundle_get_resource_path(const app_bundle* bundle, char* buffer, size_t buffer_size) {
    if (!bundle || !buffer) return NULL;
    
    snprintf(buffer, buffer_size, "%s/Contents/Resources", bundle->bundle_path);
    return buffer;
}

app_bundle* app_bundle_find_by_id(const char* bundle_id) {
    if (!bundle_id || !g_app_manager) return NULL;
    
    for (int i = 0; i < g_app_manager->num_bundles; i++) {
        if (strcmp(g_app_manager->bundles[i].bundle_identifier, bundle_id) == 0) {
            return &g_app_manager->bundles[i];
        }
    }
    return NULL;
}

app_bundle* app_bundle_find_by_name(const char* bundle_name) {
    if (!bundle_name || !g_app_manager) return NULL;
    
    for (int i = 0; i < g_app_manager->num_bundles; i++) {
        if (g_app_manager->bundles[i].bundle_name && 
            strcmp(g_app_manager->bundles[i].bundle_name, bundle_name) == 0) {
            return &g_app_manager->bundles[i];
        }
    }
    return NULL;
}

bool app_bundle_install(const char* source_path) {
    if (!source_path || !g_app_manager) return false;
    
    if (!is_valid_app_bundle(source_path)) {
        fprintf(stderr, "Not a valid app bundle: %s\n", source_path);
        return false;
    }
    
    const char* bundle_name = strrchr(source_path, '/');
    if (!bundle_name) bundle_name = source_path;
    else bundle_name++;
    
    char dest_path[MAX_BUNDLE_PATH * 2];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", 
             g_app_manager->install_directory, bundle_name);
    
    fprintf(stdout, "Installing from %s to %s\n", source_path, dest_path);
    
    if (!copy_directory_recursive(source_path, dest_path)) {
        fprintf(stderr, "Error: Failed to copy application bundle\n");
        return false;
    }
    
    app_bundle* bundle = app_bundle_parse(dest_path);
    if (!bundle) {
        fprintf(stderr, "Error: Failed to parse installed bundle\n");
        remove_directory_recursive(dest_path);
        return false;
    }
    
    // 确保 bundle_path 是正确的目标路径
    strncpy(bundle->bundle_path, dest_path, MAX_BUNDLE_PATH - 1);
    bundle->bundle_path[MAX_BUNDLE_PATH - 1] = '\0';
    
    if (g_app_manager->num_bundles >= g_app_manager->max_bundles) {
        size_t new_max = g_app_manager->max_bundles * 2;
        app_bundle* new_bundles = (app_bundle*)realloc(g_app_manager->bundles, 
            sizeof(app_bundle) * new_max);
        if (new_bundles) {
            g_app_manager->bundles = new_bundles;
            g_app_manager->max_bundles = new_max;
        }
    }
    
    if (g_app_manager->num_bundles < g_app_manager->max_bundles) {
        app_bundle* dest = &g_app_manager->bundles[g_app_manager->num_bundles];
        // 深拷贝，而不是浅拷贝
        memcpy(dest, bundle, sizeof(app_bundle));
        // 重新分配字符串，避免 double free
        dest->bundle_name = bundle->bundle_name ? strdup(bundle->bundle_name) : NULL;
        dest->bundle_version = bundle->bundle_version ? strdup(bundle->bundle_version) : NULL;
        dest->minimum_system_version = bundle->minimum_system_version ? strdup(bundle->minimum_system_version) : NULL;
        dest->icon_file = bundle->icon_file ? strdup(bundle->icon_file) : NULL;
        dest->info_plist_entries = NULL; // 暂时不处理 info_plist_entries
        dest->num_plist_entries = 0;
        
        g_app_manager->num_bundles++;
        fprintf(stdout, "Application installed successfully!\n");
    }
    
    app_bundle_free(bundle);
    return true;
}

bool app_bundle_uninstall(const char* bundle_id) {
    if (!bundle_id || !g_app_manager) return false;
    
    int index = -1;
    for (int i = 0; i < g_app_manager->num_bundles; i++) {
        if (strcmp(g_app_manager->bundles[i].bundle_identifier, bundle_id) == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        fprintf(stderr, "Error: Bundle not found: %s\n", bundle_id);
        return false;
    }
    
    char app_path[MAX_BUNDLE_PATH];
    strncpy(app_path, g_app_manager->bundles[index].bundle_path, MAX_BUNDLE_PATH - 1);
    app_path[MAX_BUNDLE_PATH - 1] = '\0';
    
    fprintf(stdout, "Uninstalling: %s\n", app_path);
    
    if (!remove_directory_recursive(app_path)) {
        fprintf(stderr, "Error: Failed to remove application files\n");
        return false;
    }
    
    for (int i = index; i < g_app_manager->num_bundles - 1; i++) {
        memcpy(&g_app_manager->bundles[i], &g_app_manager->bundles[i + 1], sizeof(app_bundle));
    }
    
    g_app_manager->num_bundles--;
    fprintf(stdout, "Application uninstalled successfully!\n");
    
    return true;
}

void app_bundle_list_installed(void) {
    if (!g_app_manager) return;
    
    fprintf(stdout, "\nInstalled Applications:\n");
    fprintf(stdout, "────────────────────────────────────────\n");
    
    if (g_app_manager->num_bundles == 0) {
        fprintf(stdout, "  No applications installed\n");
    } else {
        for (int i = 0; i < g_app_manager->num_bundles; i++) {
            app_bundle* bundle = &g_app_manager->bundles[i];
            fprintf(stdout, "  %s\n", bundle->bundle_name ? bundle->bundle_name : "Unknown");
            fprintf(stdout, "    ID:    %s\n", bundle->bundle_identifier);
            fprintf(stdout, "    Path:  %s\n", bundle->bundle_path);
            if (bundle->bundle_version) {
                fprintf(stdout, "    Ver:   %s\n", bundle->bundle_version);
            }
            if (bundle->minimum_system_version) {
                fprintf(stdout, "    Req:   macOS %s+\n", bundle->minimum_system_version);
            }
            fprintf(stdout, "\n");
        }
    }
    
    fprintf(stdout, "────────────────────────────────────────\n");
    fprintf(stdout, "Total: %zu application(s)\n\n", g_app_manager->num_bundles);
}

int app_bundle_launch(const app_bundle* bundle, int argc, char* argv[]) {
    if (!bundle || !bundle->is_valid) {
        fprintf(stderr, "Error: Cannot launch invalid bundle\n");
        return -1;
    }
    
    char exe_path[MAX_BUNDLE_PATH * 2];
    app_bundle_get_executable_path(bundle, exe_path, sizeof(exe_path));
    
    fprintf(stdout, "Launching: %s\n", exe_path);
    
    return 0;
}
