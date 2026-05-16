#include "app_bundle.h"
#include "macho.h"
#include "dyld.h"
#include "platform.h"
#include "syscall.h"
#include "vfs.h"
#include "util.h"
#include "dmg.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.8.0"

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
    fprintf(stdout, "  install <app_path>        Install an app bundle or DMG\n");
    fprintf(stdout, "  dmg-info <dmg_path>       Show DMG image information\n");
    fprintf(stdout, "  dmg-list <dmg_path>       List files in DMG image\n");
    fprintf(stdout, "  list                     List installed applications\n");
    fprintf(stdout, "  uninstall <bundle_id>     Uninstall an application\n");
    fprintf(stdout, "  version                  Show version information\n");
    fprintf(stdout, "  help                     Show this help message\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Interactive Mode (no arguments required):\n");
    fprintf(stdout, "  %s                 Launch interactive UI menu\n", program_name);
    fprintf(stdout, "  %s --gui           Launch interactive UI menu\n", program_name);
    fprintf(stdout, "  %s --interactive    Launch interactive UI menu\n", program_name);
    fprintf(stdout, "\nExamples:\n");
    fprintf(stdout, "  %s test MyApp.app/Contents/MacOS/MyApp\n", program_name);
    fprintf(stdout, "  %s install MyApp.app\n", program_name);
    fprintf(stdout, "  %s list\n", program_name);
    fprintf(stdout, "  %s uninstall com.example.MyApp\n", program_name);
    fprintf(stdout, "  %s\n", program_name);
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
    config_init();
    log_init(NULL, LOG_LEVEL_INFO);
    
    if (argc < 2 || strcmp(argv[1], "--gui") == 0 || strcmp(argv[1], "--interactive") == 0 || strcmp(argv[1], "-i") == 0) {
        print_banner();
        printf("  Starting interactive mode...\n\n");
        log_info("Starting interactive UI mode");
        int result = ui_run_interactive_mode();
        log_cleanup();
        config_cleanup();
        return result;
    }
    
    const char* command = argv[1];
    
    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "version") == 0 || strcmp(command, "--version") == 0 || strcmp(command, "-v") == 0) {
        print_version();
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "test") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Mach-O path required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* filename = argv[2];
        log_info("Testing Mach-O load: %s", filename);
        fprintf(stdout, "Testing Mach-O load: %s\n", filename);
        fprintf(stdout, "────────────────────────────────────────\n");
        
        init_syscall_table();
        vfs_init();
        dyld_init();
        
        struct mach_header* header = NULL;
        bool is_64bit = false;
        
        if (!parse_macho_header(filename, &header, &is_64bit)) {
            log_error("Failed to parse Mach-O header");
            goto cleanup;
        }
        
        fprintf(stdout, "\nMach-O Header Info:\n");
        fprintf(stdout, "  Magic:          0x%x\n", header->magic);
        fprintf(stdout, "  CPU Type:       0x%x\n", header->cputype);
        fprintf(stdout, "  File Type:      0x%x\n", header->filetype);
        fprintf(stdout, "  Number of cmds: %d\n", header->ncmds);
        fprintf(stdout, "  Size of cmds:   %d\n", header->sizeofcmds);
        fprintf(stdout, "  64-bit:         %s\n", is_64bit ? "Yes" : "No");
        
        void* base_address = NULL;
        if (!map_macho_segments(filename, header, is_64bit, &base_address)) {
            log_error("Failed to map Mach-O segments");
            goto cleanup;
        }
        
        fprintf(stdout, "\nSegment Mapping:\n");
        fprintf(stdout, "  Base address:   %p\n", base_address);
        
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
        log_info("Test completed successfully");
        
    cleanup:
        if (base_address && header) {
            unmap_macho_segments(base_address, header, is_64bit);
        } else if (header) {
            free(header);
        }
        
        dyld_cleanup();
        vfs_cleanup();
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "load") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: App bundle path required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* app_path = argv[2];
        log_info("Loading application bundle: %s", app_path);
        fprintf(stdout, "Loading application bundle: %s\n", app_path);
        
        app_bundle* bundle = app_bundle_parse(app_path);
        if (!bundle) {
            log_error("Failed to parse app bundle");
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        int result = app_bundle_launch(bundle, argc - 3, argv + 3);
        app_bundle_free(bundle);
        log_cleanup();
        config_cleanup();
        return result;
    }
    
    if (strcmp(command, "install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: App bundle path required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* app_path = argv[2];
        log_info("Installing application: %s", app_path);
        
        if (!app_manager_init(NULL)) {
            log_error("Failed to initialize app manager");
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        bool success = app_bundle_install(app_path);
        app_manager_cleanup();
        log_cleanup();
        config_cleanup();
        return success ? 0 : 1;
    }
    
    if (strcmp(command, "dmg-info") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: DMG path required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* dmg_path = argv[2];
        log_info("Showing DMG info: %s", dmg_path);
        
        dmg_context* ctx = NULL;
        if (!dmg_open(dmg_path, &ctx)) {
            fprintf(stderr, "Failed to open DMG file: %s\n", dmg_path);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        dmg_print_info(ctx);
        dmg_close(ctx);
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "dmg-list") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: DMG path required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* dmg_path = argv[2];
        log_info("Listing files in DMG: %s", dmg_path);
        
        dmg_context* ctx = NULL;
        if (!dmg_open(dmg_path, &ctx)) {
            fprintf(stderr, "Failed to open DMG file: %s\n", dmg_path);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        dmg_list_files(ctx);
        dmg_close(ctx);
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "dmg-install") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: DMG path required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* dmg_path = argv[2];
        const char* output_dir = (argc > 3) ? argv[3] : "./extracted";
        log_info("Installing from DMG: %s", dmg_path);
        
        dmg_context* ctx = NULL;
        if (!dmg_open(dmg_path, &ctx)) {
            fprintf(stderr, "Failed to open DMG file: %s\n", dmg_path);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        dmg_print_info(ctx);
        
        if (ctx->is_raw_hfs) {
            fprintf(stdout, "\nExtracting Mach-O files from raw HFS+ image...\n");
            dmg_extract_from_raw_hfs(ctx, output_dir);
        } else {
            fprintf(stdout, "\nSearching for .app bundle in DMG...\n");
            dmg_find_and_extract_app(ctx, "*.app", output_dir);
        }
        
        dmg_close(ctx);
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "list") == 0) {
        if (!app_manager_init(NULL)) {
            log_error("Failed to initialize app manager");
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        log_info("Listing installed applications");
        app_bundle_list_installed();
        app_manager_cleanup();
        log_cleanup();
        config_cleanup();
        return 0;
    }
    
    if (strcmp(command, "uninstall") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: Bundle ID required\n\n");
            print_usage(argv[0]);
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        const char* bundle_id = argv[2];
        log_info("Uninstalling application: %s", bundle_id);
        
        if (!app_manager_init(NULL)) {
            log_error("Failed to initialize app manager");
            log_cleanup();
            config_cleanup();
            return 1;
        }
        
        bool success = app_bundle_uninstall(bundle_id);
        app_manager_cleanup();
        log_cleanup();
        config_cleanup();
        return success ? 0 : 1;
    }
    
    log_error("Unknown command: %s", command);
    fprintf(stderr, "Error: Unknown command: %s\n\n", command);
    print_usage(argv[0]);
    log_cleanup();
    config_cleanup();
    return 1;
}
