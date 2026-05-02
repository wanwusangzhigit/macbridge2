
#include "app_bundle.h"
#include "macho.h"
#include "dyld.h"
#include "platform.h"
#include "syscall.h"
#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.6.1"

static void print_banner(void) {
    fprintf(stdout, "\n");
    fprintf(stdout, "  ╔════════════════════════════════════════╗\n");
    fprintf(stdout, "  ║     WinDarling Application Loader      ║\n");
    fprintf(stdout, "  ║     Version %-24s║\n", VERSION);
    fprintf(stdout, "  ╚════════════════════════════════════════╝\n");
    fprintf(stdout, "\n");
}

static void print_usage(const char* program_name) {
    print_banner();
    fprintf(stdout, "Usage: %s <command> [options]\n\n", program_name);
    fprintf(stdout, "Commands:\n");
    fprintf(stdout, "  test <macho_path>         Test loading a Mach-O file\n");
    fprintf(stdout, "  load <app_path> [args]   Load and run an app bundle\n");
    fprintf(stdout, "  install <app_path>       Install an app bundle\n");
    fprintf(stdout, "  list                     List installed applications\n");
    fprintf(stdout, "  uninstall <bundle_id>     Uninstall an application\n");
    fprintf(stdout, "  version                  Show version information\n");
    fprintf(stdout, "  help                     Show this help message\n");
    fprintf(stdout, "\nExamples:\n");
    fprintf(stdout, "  %s test MyApp.app/Contents/MacOS/MyApp\n", program_name);
    fprintf(stdout, "  %s install MyApp.app\n", program_name);
    fprintf(stdout, "  %s list\n", program_name);
    fprintf(stdout, "  %s uninstall com.example.MyApp\n", program_name);
    fprintf(stdout, "\nFor more information, see README.md\n");
}

static void print_version(void) {
    print_banner();
    fprintf(stdout, "WinDarling Application Loader\n");
    fprintf(stdout, "Version: %s\n", VERSION);
    fprintf(stdout, "Build date: %s %s\n", __DATE__, __TIME__);
    fprintf(stdout, "\nFeatures:\n");
    fprintf(stdout, "  - Mach-O file parsing\n");
    fprintf(stdout, "  - App bundle management\n");
    fprintf(stdout, "  - Virtual filesystem\n");
    fprintf(stdout, "  - System call translation\n");
    fprintf(stdout, "  - Dynamic linker simulation\n");
    fprintf(stdout, "\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* command = argv[1];
    
    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(command, "version") == 0 || strcmp(command, "--version") == 0 || strcmp(command, "-v") == 0) {
        print_version();
        return 0;
    }
    
    if (strcmp(command, "test") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Mach-O path required\n\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* filename = argv[2];
        fprintf(stdout, "Testing Mach-O load: %s\n", filename);
        fprintf(stdout, "────────────────────────────────────────\n");
        
        // 初始化各个子系统
        init_syscall_table();
        vfs_init();
        dyld_init();
        
        struct mach_header* header = NULL;
        bool is_64bit = false;
        
        // 解析 Mach-O 头部
        if (!parse_macho_header(filename, &header, &is_64bit)) {
            fprintf(stderr, "\nError: Failed to parse Mach-O header\n");
            goto cleanup;
        }
        
        fprintf(stdout, "\nMach-O Header Info:\n");
        fprintf(stdout, "  Magic:          0x%x\n", header->magic);
        fprintf(stdout, "  CPU Type:       0x%x\n", header->cputype);
        fprintf(stdout, "  File Type:      0x%x\n", header->filetype);
        fprintf(stdout, "  Number of cmds: %d\n", header->ncmds);
        fprintf(stdout, "  Size of cmds:   %d\n", header->sizeofcmds);
        fprintf(stdout, "  64-bit:         %s\n", is_64bit ? "Yes" : "No");
        
        // 映射段到内存
        void* base_address = NULL;
        if (!map_macho_segments(filename, header, is_64bit, &base_address)) {
            fprintf(stderr, "\nError: Failed to map Mach-O segments\n");
            goto cleanup;
        }
        
        fprintf(stdout, "\nSegment Mapping:\n");
        fprintf(stdout, "  Base address:   %p\n", base_address);
        
        // 查找 main 函数
        void* main_addr = find_main_function(filename, header, is_64bit, base_address);
        fprintf(stdout, "\nMain Function:\n");
        if (main_addr) {
            fprintf(stdout, "  Address:        %p\n", main_addr);
            fprintf(stdout, "  Status:         Found\n");
        } else {
            fprintf(stdout, "  Status:         Not found (this is normal for some Mach-O files)\n");
        }
        
        fprintf(stdout, "\n────────────────────────────────────────\n");
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
            fprintf(stderr, "Error: App bundle path required\n\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* app_path = argv[2];
        fprintf(stdout, "Loading application bundle: %s\n", app_path);
        
        app_bundle* bundle = app_bundle_parse(app_path);
        if (!bundle) {
            fprintf(stderr, "Error: Failed to parse app bundle\n");
            return 1;
        }
        
        int result = app_bundle_launch(bundle, argc - 3, argv + 3);
        app_bundle_free(bundle);
        return result;
    }
    
    if (strcmp(command, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: App bundle path required\n\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* app_path = argv[2];
        
        if (!app_manager_init(NULL)) {
            fprintf(stderr, "Error: Failed to initialize app manager\n");
            return 1;
        }
        
        bool success = app_bundle_install(app_path);
        app_manager_cleanup();
        return success ? 0 : 1;
    }
    
    if (strcmp(command, "list") == 0) {
        if (!app_manager_init(NULL)) {
            fprintf(stderr, "Error: Failed to initialize app manager\n");
            return 1;
        }
        
        app_bundle_list_installed();
        app_manager_cleanup();
        return 0;
    }
    
    if (strcmp(command, "uninstall") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Bundle ID required\n\n");
            print_usage(argv[0]);
            return 1;
        }
        
        const char* bundle_id = argv[2];
        
        if (!app_manager_init(NULL)) {
            fprintf(stderr, "Error: Failed to initialize app manager\n");
            return 1;
        }
        
        bool success = app_bundle_uninstall(bundle_id);
        app_manager_cleanup();
        return success ? 0 : 1;
    }
    
    fprintf(stderr, "Error: Unknown command: %s\n\n", command);
    print_usage(argv[0]);
    return 1;
}
