
#include "app_bundle.h"
#include "vfs.h"
#include "macho.h"
#include &lt;stdio.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;string.h&gt;
#include &lt;ctype.h&gt;

#ifdef _WIN32
#include &lt;windows.h&gt;
#include &lt;direct.h&gt;
#define mkdir(path) _mkdir(path)
#define S_ISDIR(m) 0
#else
#include &lt;unistd.h&gt;
#include &lt;dirent.h&gt;
#include &lt;sys/stat.h&gt;
#endif

// 全局应用包管理器
static app_manager* g_app_manager = NULL;

// ======== 内部辅助函数 ========

static char* str_dup(const char* str) {
    if (!str) return NULL;
    char* dup = (char*)malloc(strlen(str) + 1);
    if (dup) strcpy(dup, str);
    return dup;
}

static void free_plist_entries(info_plist_entry* entries) {
    info_plist_entry* current = entries;
    while (current) {
        info_plist_entry* next = (info_plist_entry*)current-&gt;next;
        free(current-&gt;key);
        free(current-&gt;value);
        free(current);
        current = next;
    }
}

static bool file_exists(const char* path) {
#ifdef _WIN32
    DWORD attribs = GetFileAttributesA(path);
    return (attribs != INVALID_FILE_ATTRIBUTES);
#else
    struct stat st;
    return stat(path, &amp;st) == 0;
#endif
}

static bool is_dir(const char* path) {
#ifdef _WIN32
    DWORD attribs = GetFileAttributesA(path);
    return (attribs != INVALID_FILE_ATTRIBUTES &amp;&amp; 
            (attribs &amp; FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return stat(path, &amp;st) == 0 &amp;&amp; S_ISDIR(st.st_mode);
#endif
}

static void build_path(char* buffer, size_t buffer_size, const char* dir, const char* file) {
    snprintf(buffer, buffer_size, "%s/%s", dir, file);
}

// ======== 应用包管理器函数 ========

bool app_manager_init(const char* install_dir) {
    if (g_app_manager) {
        fprintf(stderr, "App manager already initialized\n");
        return false;
    }

    g_app_manager = (app_manager*)calloc(1, sizeof(app_manager));
    if (!g_app_manager) {
        fprintf(stderr, "Failed to allocate app manager\n");
        return false;
    }

    strncpy(g_app_manager-&gt;install_directory, install_dir, MAX_BUNDLE_PATH - 1);
    g_app_manager-&gt;install_directory[MAX_BUNDLE_PATH - 1] = '\0';
    
    g_app_manager-&gt;max_bundles = 64;
    g_app_manager-&gt;num_bundles = 0;
    g_app_manager-&gt;bundles = (app_bundle*)calloc(g_app_manager-&gt;max_bundles, sizeof(app_bundle));
    
    if (!g_app_manager-&gt;bundles) {
        free(g_app_manager);
        g_app_manager = NULL;
        fprintf(stderr, "Failed to allocate bundle array\n");
        return false;
    }

    // 确保安装目录存在
    if (!file_exists(g_app_manager-&gt;install_directory)) {
        mkdir(g_app_manager-&gt;install_directory);
    }

    printf("App manager initialized with install directory: %s\n", g_app_manager-&gt;install_directory);
    return true;
}

void app_manager_cleanup(void) {
    if (!g_app_manager) return;

    for (size_t i = 0; i &lt; g_app_manager-&gt;num_bundles; i++) {
        app_bundle_free(&amp;g_app_manager-&gt;bundles[i]);
    }
    free(g_app_manager-&gt;bundles);
    free(g_app_manager);
    g_app_manager = NULL;
    printf("App manager cleaned up\n");
}

// ======== Info.plist 解析函数 ========

bool extract_plist_key_value(const char* plist_content, const char* key, char* value, size_t value_size) {
    const char* key_tag_start = strstr(plist_content, key);
    if (!key_tag_start) return false;

    const char* key_tag_end = strchr(key_tag_start, '&gt;');
    if (!key_tag_end) return false;

    const char* value_start = strchr(key_tag_end, '&gt;');
    if (!value_start) return false;
    value_start++;

    const char* value_end = strchr(value_start, '&lt;');
    if (!value_end) return false;

    size_t value_len = value_end - value_start;
    if (value_len &gt;= value_size) value_len = value_size - 1;

    strncpy(value, value_start, value_len);
    value[value_len] = '\0';
    return true;
}

bool parse_info_plist(const char* plist_path, app_bundle* bundle) {
    FILE* fp = fopen(plist_path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open Info.plist: %s\n", plist_path);
        return false;
    }

    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(fp);
        return false;
    }

    fread(content, 1, file_size, fp);
    content[file_size] = '\0';
    fclose(fp);

    // 提取 CFBundleExecutable
    char executable[MAX_EXECUTABLE_NAME];
    if (extract_plist_key_value(content, "CFBundleExecutable", executable, sizeof(executable))) {
        strncpy(bundle-&gt;executable_name, executable, MAX_EXECUTABLE_NAME - 1);
        bundle-&gt;executable_name[MAX_EXECUTABLE_NAME - 1] = '\0';
    }

    // 提取 CFBundleIdentifier
    char bundle_id[MAX_BUNDLE_ID];
    if (extract_plist_key_value(content, "CFBundleIdentifier", bundle_id, sizeof(bundle_id))) {
        strncpy(bundle-&gt;bundle_identifier, bundle_id, MAX_BUNDLE_ID - 1);
        bundle-&gt;bundle_identifier[MAX_BUNDLE_ID - 1] = '\0';
    }

    // 提取 CFBundleName
    char bundle_name[MAX_BUNDLE_ID];
    if (extract_plist_key_value(content, "CFBundleName", bundle_name, sizeof(bundle_name))) {
        bundle-&gt;bundle_name = str_dup(bundle_name);
    }

    // 提取 CFBundleShortVersionString
    char version[MAX_BUNDLE_ID];
    if (extract_plist_key_value(content, "CFBundleShortVersionString", version, sizeof(version))) {
        bundle-&gt;bundle_version = str_dup(version);
    }

    // 提取 CFBundleIconFile
    char icon[MAX_BUNDLE_ID];
    if (extract_plist_key_value(content, "CFBundleIconFile", icon, sizeof(icon))) {
        bundle-&gt;icon_file = str_dup(icon);
    }

    // 提取 LSMinimumSystemVersion
    char min_version[MAX_BUNDLE_ID];
    if (extract_plist_key_value(content, "LSMinimumSystemVersion", min_version, sizeof(min_version))) {
        bundle-&gt;minimum_system_version = str_dup(min_version);
    }

    free(content);
    bundle-&gt;is_valid = true;
    return true;
}

bool is_valid_app_bundle(const char* path) {
    if (!is_dir(path)) return false;

    char plist_path[MAX_BUNDLE_PATH];
    build_path(plist_path, sizeof(plist_path), path, "Contents/Info.plist");
    
    if (!file_exists(plist_path)) return false;

    char contents_macos[MAX_BUNDLE_PATH];
    build_path(contents_macos, sizeof(contents_macos), path, "Contents/MacOS");
    
    return is_dir(contents_macos);
}

app_bundle* app_bundle_parse(const char* bundle_path) {
    if (!is_valid_app_bundle(bundle_path)) {
        fprintf(stderr, "Not a valid .app bundle: %s\n", bundle_path);
        return NULL;
    }

    app_bundle* bundle = (app_bundle*)calloc(1, sizeof(app_bundle));
    if (!bundle) {
        fprintf(stderr, "Failed to allocate bundle\n");
        return NULL;
    }

    strncpy(bundle-&gt;bundle_path, bundle_path, MAX_BUNDLE_PATH - 1);
    bundle-&gt;bundle_path[MAX_BUNDLE_PATH - 1] = '\0';

    char plist_path[MAX_BUNDLE_PATH];
    build_path(plist_path, sizeof(plist_path), bundle_path, "Contents/Info.plist");

    if (!parse_info_plist(plist_path, bundle)) {
        app_bundle_free(bundle);
        return NULL;
    }

    printf("Successfully parsed app bundle: %s\n", bundle_path);
    return bundle;
}

void app_bundle_free(app_bundle* bundle) {
    if (!bundle) return;

    free(bundle-&gt;bundle_name);
    free(bundle-&gt;bundle_version);
    free(bundle-&gt;minimum_system_version);
    free(bundle-&gt;icon_file);
    free_plist_entries(bundle-&gt;info_plist_entries);
}

const char* app_bundle_get_info_value(const app_bundle* bundle, const char* key) {
    if (!bundle || !key) return NULL;

    info_plist_entry* current = bundle-&gt;info_plist_entries;
    while (current) {
        if (strcmp(current-&gt;key, key) == 0) {
            return current-&gt;value;
        }
        current = (info_plist_entry*)current-&gt;next;
    }
    return NULL;
}

char* app_bundle_get_executable_path(const app_bundle* bundle, char* buffer, size_t buffer_size) {
    if (!bundle || !buffer || buffer_size == 0) return NULL;

    build_path(buffer, buffer_size, bundle-&gt;bundle_path, "Contents/MacOS");
    strncat(buffer, "/", buffer_size - strlen(buffer) - 1);
    strncat(buffer, bundle-&gt;executable_name, buffer_size - strlen(buffer) - 1);
    return buffer;
}

char* app_bundle_get_resource_path(const app_bundle* bundle, char* buffer, size_t buffer_size) {
    if (!bundle || !buffer || buffer_size == 0) return NULL;

    build_path(buffer, buffer_size, bundle-&gt;bundle_path, "Contents/Resources");
    return buffer;
}

// ======== 应用包安装函数 ========

bool app_bundle_install(const char* source_path) {
    if (!g_app_manager) {
        fprintf(stderr, "App manager not initialized\n");
        return false;
    }

    app_bundle* bundle = app_bundle_parse(source_path);
    if (!bundle) {
        fprintf(stderr, "Failed to parse app bundle\n");
        return false;
    }

    printf("Installing app bundle: %s (ID: %s)\n", 
           bundle-&gt;bundle_name ? bundle-&gt;bundle_name : bundle-&gt;executable_name,
           bundle-&gt;bundle_identifier);

    if (g_app_manager-&gt;num_bundles &gt;= g_app_manager-&gt;max_bundles) {
        fprintf(stderr, "Too many installed apps\n");
        app_bundle_free(bundle);
        free(bundle);
        return false;
    }

    // 复制应用到安装目录（简化处理，真实项目需要实际复制）
    char dest_path[MAX_BUNDLE_PATH];
    const char* bundle_name = bundle-&gt;bundle_name ? bundle-&gt;bundle_name : bundle-&gt;executable_name;
    build_path(dest_path, sizeof(dest_path), g_app_manager-&gt;install_directory, bundle_name);
    
    // 保存到管理器中
    memcpy(&amp;g_app_manager-&gt;bundles[g_app_manager-&gt;num_bundles], bundle, sizeof(app_bundle));
    // 修复路径（实际项目应该复制后更新路径）
    strncpy(g_app_manager-&gt;bundles[g_app_manager-&gt;num_bundles].bundle_path, 
            dest_path, MAX_BUNDLE_PATH - 1);
    
    g_app_manager-&gt;num_bundles++;

    printf("App bundle installed successfully: %s\n", bundle-&gt;bundle_name);
    free(bundle);
    return true;
}

bool app_bundle_uninstall(const char* bundle_id) {
    if (!g_app_manager) return false;

    for (size_t i = 0; i &lt; g_app_manager-&gt;num_bundles; i++) {
        if (strcmp(g_app_manager-&gt;bundles[i].bundle_identifier, bundle_id) == 0) {
            app_bundle_free(&amp;g_app_manager-&gt;bundles[i]);
            
            // 移动后面的应用
            if (i &lt; g_app_manager-&gt;num_bundles - 1) {
                memmove(&amp;g_app_manager-&gt;bundles[i], 
                        &amp;g_app_manager-&gt;bundles[i + 1],
                        (g_app_manager-&gt;num_bundles - i - 1) * sizeof(app_bundle));
            }
            g_app_manager-&gt;num_bundles--;
            printf("Uninstalled app with ID: %s\n", bundle_id);
            return true;
        }
    }
    fprintf(stderr, "App not found: %s\n", bundle_id);
    return false;
}

void app_bundle_list_installed(void) {
    if (!g_app_manager) {
        printf("App manager not initialized\n");
        return;
    }

    printf("\n========================================\n");
    printf("  Installed Applications (%zu)\n", g_app_manager-&gt;num_bundles);
    printf("========================================\n");

    if (g_app_manager-&gt;num_bundles == 0) {
        printf("  No applications installed\n");
        printf("========================================\n\n");
        return;
    }

    for (size_t i = 0; i &lt; g_app_manager-&gt;num_bundles; i++) {
        const app_bundle* bundle = &amp;g_app_manager-&gt;bundles[i];
        printf("\nApp #%zu:\n", i + 1);
        printf("  Name:        %s\n", bundle-&gt;bundle_name ? bundle-&gt;bundle_name : "(unknown)");
        printf("  Executable:  %s\n", bundle-&gt;executable_name);
        printf("  Bundle ID:   %s\n", bundle-&gt;bundle_identifier);
        printf("  Version:     %s\n", bundle-&gt;bundle_version ? bundle-&gt;bundle_version : "(unknown)");
        printf("  Path:        %s\n", bundle-&gt;bundle_path);
    }
    printf("\n========================================\n\n");
}

app_bundle* app_bundle_find_by_id(const char* bundle_id) {
    if (!g_app_manager || !bundle_id) return NULL;

    for (size_t i = 0; i &lt; g_app_manager-&gt;num_bundles; i++) {
        if (strcmp(g_app_manager-&gt;bundles[i].bundle_identifier, bundle_id) == 0) {
            return &amp;g_app_manager-&gt;bundles[i];
        }
    }
    return NULL;
}

app_bundle* app_bundle_find_by_name(const char* bundle_name) {
    if (!g_app_manager || !bundle_name) return NULL;

    for (size_t i = 0; i &lt; g_app_manager-&gt;num_bundles; i++) {
        if (g_app_manager-&gt;bundles[i].bundle_name &amp;&amp;
            strcmp(g_app_manager-&gt;bundles[i].bundle_name, bundle_name) == 0) {
            return &amp;g_app_manager-&gt;bundles[i];
        }
    }
    return NULL;
}

int app_bundle_launch(const app_bundle* bundle, int argc, char* argv[]) {
    if (!bundle || !bundle-&gt;is_valid) {
        fprintf(stderr, "Invalid app bundle\n");
        return -1;
    }

    printf("\n========================================\n");
    printf("  Launching Application\n");
    printf("========================================\n");
    printf("  Name:        %s\n", bundle-&gt;bundle_name ? bundle-&gt;bundle_name : bundle-&gt;executable_name);
    printf("  Bundle ID:   %s\n", bundle-&gt;bundle_identifier);
    printf("========================================\n\n");

    char executable_path[MAX_BUNDLE_PATH];
    app_bundle_get_executable_path(bundle, executable_path, sizeof(executable_path));

    printf("Executable path: %s\n", executable_path);

    // 使用现有的 Mach-O 加载器
    struct mach_header* header = NULL;
    bool is_64bit = false;
    void* base_address = NULL;

    // 初始化子系统
    init_syscall_table();
    vfs_init();
    dyld_init();

    if (!parse_macho_header(executable_path, &amp;header, &amp;is_64bit)) {
        fprintf(stderr, "Failed to parse Mach-O header\n");
        goto cleanup;
    }

    printf("Mach-O file type: %sbit\n", is_64bit ? "64" : "32");

    if (!map_macho_segments(executable_path, header, is_64bit, &amp;base_address)) {
        fprintf(stderr, "Failed to map Mach-O segments\n");
        goto cleanup;
    }

    void* main_addr = find_main_function(executable_path, header, is_64bit, base_address);
    if (!main_addr) {
        fprintf(stderr, "Failed to find main function\n");
        goto cleanup;
    }

    printf("Main function found, executing...\n");
    int result = execute_main_function(main_addr, argc, argv);
    printf("Application exited with status: %d\n", result);

cleanup:
    if (base_address &amp;&amp; header) {
        unmap_macho_segments(base_address, header, is_64bit);
    } else if (header) {
        free(header);
    }

    dyld_cleanup();
    vfs_cleanup();

    return 0;
}

