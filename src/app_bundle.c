#include "app_bundle.h"
#include "platform.h"
#include "macho.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    free(entry);
}

static bool extract_plist_key_value(const char* content, const char* key, char* value, size_t value_size) {
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

static bool parse_info_plist(const char* plist_path, app_bundle* bundle) {
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
        bundle->bundle_name = strdup(buffer);
    }
    
    if (extract_plist_key_value(content, "CFBundleVersion", buffer, sizeof(buffer))) {
        bundle->bundle_version = strdup(buffer);
    }
    
    if (extract_plist_key_value(content, "LSMinimumSystemVersion", buffer, sizeof(buffer))) {
        bundle->minimum_system_version = strdup(buffer);
    }
    
    if (extract_plist_key_value(content, "CFBundleIconFile", buffer, sizeof(buffer))) {
        bundle->icon_file = strdup(buffer);
    }
    
    free(content);
    return true;
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

static bool app_bundle_save_to_file(const app_bundle* bundle, const char* file_path) {
    FILE* file = fopen(file_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file for writing: %s\n", file_path);
        return false;
    }
    
    if (fwrite(bundle->bundle_identifier, 1, strlen(bundle->bundle_identifier) + 1, file) != strlen(bundle->bundle_identifier) + 1) {
        fclose(file);
        return false;
    }
    
    if (fwrite(bundle->bundle_path, 1, strlen(bundle->bundle_path) + 1, file) != strlen(bundle->bundle_path) + 1) {
        fclose(file);
        return false;
    }
    
    size_t name_len = bundle->bundle_name ? strlen(bundle->bundle_name) + 1 : 1;
    if (fwrite(&name_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (bundle->bundle_name) {
        if (fwrite(bundle->bundle_name, sizeof(char), name_len, file) != name_len) { fclose(file); return false; }
    } else {
        char null_char = '\0';
        if (fwrite(&null_char, sizeof(char), 1, file) != 1) { fclose(file); return false; }
    }
    
    size_t version_len = bundle->bundle_version ? strlen(bundle->bundle_version) + 1 : 1;
    if (fwrite(&version_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (bundle->bundle_version) {
        if (fwrite(bundle->bundle_version, sizeof(char), version_len, file) != version_len) { fclose(file); return false; }
    } else {
        char null_char = '\0';
        if (fwrite(&null_char, sizeof(char), 1, file) != 1) { fclose(file); return false; }
    }
    
    size_t sys_ver_len = bundle->minimum_system_version ? strlen(bundle->minimum_system_version) + 1 : 1;
    if (fwrite(&sys_ver_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (bundle->minimum_system_version) {
        if (fwrite(bundle->minimum_system_version, sizeof(char), sys_ver_len, file) != sys_ver_len) { fclose(file); return false; }
    } else {
        char null_char = '\0';
        if (fwrite(&null_char, sizeof(char), 1, file) != 1) { fclose(file); return false; }
    }
    
    size_t icon_len = bundle->icon_file ? strlen(bundle->icon_file) + 1 : 1;
    if (fwrite(&icon_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (bundle->icon_file) {
        if (fwrite(bundle->icon_file, sizeof(char), icon_len, file) != icon_len) { fclose(file); return false; }
    } else {
        char null_char = '\0';
        if (fwrite(&null_char, sizeof(char), 1, file) != 1) { fclose(file); return false; }
    }
    
    fclose(file);
    return true;
}

static bool app_bundle_load_from_file(const char* file_path, app_bundle* bundle) {
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return false;
    }
    
    char buffer[4096];
    
    if (fread(buffer, 1, MAX_BUNDLE_ID - 1, file) == 0) { fclose(file); return false; }
    buffer[MAX_BUNDLE_ID - 1] = '\0';
    strncpy(bundle->bundle_identifier, buffer, MAX_BUNDLE_ID - 1);
    
    if (fread(buffer, 1, MAX_BUNDLE_PATH - 1, file) == 0) { fclose(file); return false; }
    buffer[MAX_BUNDLE_PATH - 1] = '\0';
    strncpy(bundle->bundle_path, buffer, MAX_BUNDLE_PATH - 1);
    
    size_t name_len = 0;
    if (fread(&name_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (name_len > 0) {
        bundle->bundle_name = (char*)malloc(name_len);
        if (bundle->bundle_name) {
            if (fread(bundle->bundle_name, sizeof(char), name_len, file) != name_len) {
                free(bundle->bundle_name);
                bundle->bundle_name = NULL;
            }
        }
    }
    
    size_t version_len = 0;
    if (fread(&version_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (version_len > 0) {
        bundle->bundle_version = (char*)malloc(version_len);
        if (bundle->bundle_version) {
            if (fread(bundle->bundle_version, sizeof(char), version_len, file) != version_len) {
                free(bundle->bundle_version);
                bundle->bundle_version = NULL;
            }
        }
    }
    
    size_t sys_ver_len = 0;
    if (fread(&sys_ver_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (sys_ver_len > 0) {
        bundle->minimum_system_version = (char*)malloc(sys_ver_len);
        if (bundle->minimum_system_version) {
            if (fread(bundle->minimum_system_version, sizeof(char), sys_ver_len, file) != sys_ver_len) {
                free(bundle->minimum_system_version);
                bundle->minimum_system_version = NULL;
            }
        }
    }
    
    size_t icon_len = 0;
    if (fread(&icon_len, sizeof(size_t), 1, file) != 1) { fclose(file); return false; }
    if (icon_len > 0) {
        bundle->icon_file = (char*)malloc(icon_len);
        if (bundle->icon_file) {
            if (fread(bundle->icon_file, sizeof(char), icon_len, file) != icon_len) {
                free(bundle->icon_file);
                bundle->icon_file = NULL;
            }
        }
    }
    
    fclose(file);
    return true;
}

int app_manager_init(const char* install_directory) {
    if (g_app_manager) {
        return 1;
    }
    
    g_app_manager = (app_manager*)malloc(sizeof(app_manager));
    if (!g_app_manager) {
        return 0;
    }
    
    if (install_directory) {
        strncpy(g_app_manager->install_directory, install_directory, MAX_BUNDLE_PATH - 1);
    } else {
        snprintf(g_app_manager->install_directory, MAX_BUNDLE_PATH, "./Applications");
    }
    
    create_directory_recursive(g_app_manager->install_directory);
    
    g_app_manager->bundles = NULL;
    g_app_manager->bundle_count = 0;
    g_app_manager->max_bundles = 16;
    g_app_manager->bundles = (app_bundle*)malloc(sizeof(app_bundle) * g_app_manager->max_bundles);
    
    if (!g_app_manager->bundles) {
        free(g_app_manager);
        g_app_manager = NULL;
        return 0;
    }
    
    char db_path[MAX_BUNDLE_PATH];
    snprintf(db_path, sizeof(db_path), "%s/.app_db", g_app_manager->install_directory);
    
    FILE* db_file = fopen(db_path, "rb");
    if (db_file) {
        int count;
        if (fread(&count, sizeof(int), 1, db_file) == 1) {
            for (int i = 0; i < count && i < g_app_manager->max_bundles; i++) {
                app_bundle_load_from_file(db_path, &g_app_manager->bundles[i]);
                g_app_manager->bundle_count++;
            }
        }
        fclose(db_file);
    }
    
    return 1;
}

void app_manager_cleanup(void) {
    if (!g_app_manager) return;
    
    char db_path[MAX_BUNDLE_PATH];
    snprintf(db_path, sizeof(db_path), "%s/.app_db", g_app_manager->install_directory);
    
    FILE* db_file = fopen(db_path, "wb");
    if (db_file) {
        fwrite(&g_app_manager->bundle_count, sizeof(int), 1, db_file);
        for (int i = 0; i < g_app_manager->bundle_count; i++) {
            app_bundle_save_to_file(&g_app_manager->bundles[i], db_path);
        }
        fclose(db_file);
    }
    
    for (int i = 0; i < g_app_manager->bundle_count; i++) {
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
    bundle->bundle_path[MAX_BUNDLE_PATH - 1] = '\0';
    
    char info_plist_path[MAX_BUNDLE_PATH];
    snprintf(info_plist_path, sizeof(info_plist_path), "%s/Contents/Info.plist", bundle_path);
    
    if (parse_info_plist(info_plist_path, bundle)) {
        bundle->is_valid = true;
    }
    
    return bundle;
}

void app_bundle_free(app_bundle* bundle) {
    if (!bundle) return;
    free(bundle->bundle_name);
    free(bundle->bundle_version);
    free(bundle->minimum_system_version);
    free(bundle->icon_file);
    free(bundle);
}

void app_bundle_get_executable_path(const app_bundle* bundle, char* path, size_t path_size) {
    if (!bundle || !path) return;
    
    snprintf(path, path_size, "%s/Contents/MacOS/%s", bundle->bundle_path, bundle->executable_name);
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

bool app_bundle_install(const char* source_path) {
    if (!source_path || !g_app_manager) return false;
    
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
    
    if (g_app_manager->bundle_count >= g_app_manager->max_bundles) {
        app_bundle** new_bundles = (app_bundle**)realloc(g_app_manager->bundles, 
            sizeof(app_bundle*) * g_app_manager->max_bundles * 2);
        if (new_bundles) {
            g_app_manager->bundles = (app_bundle*)new_bundles;
            g_app_manager->max_bundles *= 2;
        }
    }
    
    if (g_app_manager->bundle_count < g_app_manager->max_bundles) {
        memcpy(&g_app_manager->bundles[g_app_manager->bundle_count], bundle, sizeof(app_bundle));
        g_app_manager->bundle_count++;
        fprintf(stdout, "Application installed successfully!\n");
    }
    
    free(bundle);
    return true;
}

bool app_bundle_uninstall(const char* bundle_id) {
    if (!bundle_id || !g_app_manager) return false;
    
    int index = -1;
    for (int i = 0; i < g_app_manager->bundle_count; i++) {
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
    
    for (int i = index; i < g_app_manager->bundle_count - 1; i++) {
        memcpy(&g_app_manager->bundles[i], &g_app_manager->bundles[i + 1], sizeof(app_bundle));
    }
    
    g_app_manager->bundle_count--;
    fprintf(stdout, "Application uninstalled successfully!\n");
    
    return true;
}

void app_bundle_list_installed(void) {
    if (!g_app_manager) return;
    
    fprintf(stdout, "\nInstalled Applications:\n");
    fprintf(stdout, "────────────────────────────────────────\n");
    
    if (g_app_manager->bundle_count == 0) {
        fprintf(stdout, "  No applications installed\n");
    } else {
        for (int i = 0; i < g_app_manager->bundle_count; i++) {
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
    fprintf(stdout, "Total: %d application(s)\n\n", g_app_manager->bundle_count);
}
