
#pragma once

#include &lt;stdint.h&gt;
#include &lt;stdbool.h&gt;
#include &lt;stddef.h&gt;

#define MAX_BUNDLE_ID 256
#define MAX_EXECUTABLE_NAME 256
#define MAX_BUNDLE_PATH 512

// Info.plist 键值对
typedef struct {
    char* key;
    char* value;
    struct info_plist_entry* next;
} info_plist_entry;

// .app 应用包结构
typedef struct {
    char bundle_path[MAX_BUNDLE_PATH];              // 包路径
    char executable_name[MAX_EXECUTABLE_NAME];      // 可执行文件名
    char bundle_identifier[MAX_BUNDLE_ID];          // 包标识符
    char* bundle_name;                              // 包名称
    char* bundle_version;                           // 包版本
    char* minimum_system_version;                   // 最低系统版本
    char* icon_file;                                // 图标文件
    bool is_valid;                                  // 包是否有效
    info_plist_entry* info_plist_entries;           // Info.plist 条目
    size_t num_plist_entries;                       // 条目数量
} app_bundle;

// 应用包管理器结构
typedef struct {
    app_bundle* bundles;                            // 已安装的应用包列表
    size_t num_bundles;                             // 已安装的应用包数量
    size_t max_bundles;                             // 最大容量
    char install_directory[MAX_BUNDLE_PATH];        // 安装目录
} app_manager;

// ======== 应用包管理函数 ========

// 初始化应用包管理器
bool app_manager_init(const char* install_dir);

// 清理应用包管理器
void app_manager_cleanup(void);

// 解析应用包
app_bundle* app_bundle_parse(const char* bundle_path);

// 释放应用包资源
void app_bundle_free(app_bundle* bundle);

// 从应用包获取 Info.plist 值
const char* app_bundle_get_info_value(const app_bundle* bundle, const char* key);

// 获取应用包的可执行文件路径
char* app_bundle_get_executable_path(const app_bundle* bundle, char* buffer, size_t buffer_size);

// 获取应用包资源路径
char* app_bundle_get_resource_path(const app_bundle* bundle, char* buffer, size_t buffer_size);

// 安装应用包
bool app_bundle_install(const char* source_path);

// 卸载应用包
bool app_bundle_uninstall(const char* bundle_id);

// 列出所有已安装的应用
void app_bundle_list_installed(void);

// 查找已安装的应用包
app_bundle* app_bundle_find_by_id(const char* bundle_id);

// 查找已安装的应用包（按名称）
app_bundle* app_bundle_find_by_name(const char* bundle_name);

// 启动应用包中的可执行文件
int app_bundle_launch(const app_bundle* bundle, int argc, char* argv[]);

// ======== 实用工具函数 ========

// 检查路径是否为有效的 .app 包
bool is_valid_app_bundle(const char* path);

// 解析 Info.plist（简化版）
bool parse_info_plist(const char* plist_path, app_bundle* bundle);

// 从 Info.plist 提取键值对
bool extract_plist_key_value(const char* plist_content, const char* key, char* value, size_t value_size);

