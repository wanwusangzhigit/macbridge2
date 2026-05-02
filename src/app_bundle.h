
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_BUNDLE_ID 256
#define MAX_EXECUTABLE_NAME 256
#define MAX_BUNDLE_PATH 512

typedef struct {
    char* key;
    char* value;
    struct info_plist_entry* next;
} info_plist_entry;

typedef struct {
    char bundle_path[MAX_BUNDLE_PATH];
    char executable_name[MAX_EXECUTABLE_NAME];
    char bundle_identifier[MAX_BUNDLE_ID];
    char* bundle_name;
    char* bundle_version;
    char* minimum_system_version;
    char* icon_file;
    bool is_valid;
    info_plist_entry* info_plist_entries;
    size_t num_plist_entries;
} app_bundle;

typedef struct {
    app_bundle* bundles;
    size_t num_bundles;
    size_t max_bundles;
    char install_directory[MAX_BUNDLE_PATH];
} app_manager;

bool app_manager_init(const char* install_dir);
void app_manager_cleanup(void);
bool app_manager_save(void);
bool app_manager_load(void);
app_bundle* app_bundle_parse(const char* bundle_path);
void app_bundle_free(app_bundle* bundle);
const char* app_bundle_get_info_value(const app_bundle* bundle, const char* key);
char* app_bundle_get_executable_path(const app_bundle* bundle, char* buffer, size_t buffer_size);
char* app_bundle_get_resource_path(const app_bundle* bundle, char* buffer, size_t buffer_size);
bool app_bundle_install(const char* source_path);
bool app_bundle_uninstall(const char* bundle_id);
void app_bundle_list_installed(void);
app_bundle* app_bundle_find_by_id(const char* bundle_id);
app_bundle* app_bundle_find_by_name(const char* bundle_name);
int app_bundle_launch(const app_bundle* bundle, int argc, char* argv[]);
bool is_valid_app_bundle(const char* path);
bool parse_info_plist(const char* plist_path, app_bundle* bundle);
bool extract_plist_key_value(const char* plist_content, const char* key, char* value, size_t value_size);

