#include "ui.h"
#include "app_bundle.h"
#include "dmg.h"
#include "platform.h"
#include "macho.h"
#include "dyld.h"
#include "syscall.h"
#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define MAX_PATH_LEN MAX_PATH
#define CLEAR_SCREEN() do { int _r = system("cls"); (void)_r; } while(0)
#define GETCH() _getch()
#define KBHIT() _kbhit()
#else
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#define MAX_PATH_LEN 1024
#define CLEAR_SCREEN() do { int _r = system("clear"); (void)_r; } while(0)
#define GETCH() getchar()
#define KBHIT() 0
#endif

#define MAX_MENU_ITEMS 20
#define MAX_LINE_LENGTH 256

static char input_buffer[MAX_LINE_LENGTH];

static void print_centered_text(const char* text, int width) {
    int len = (int)strlen(text);
    int padding = (width - len) / 2;
    if (padding < 0) padding = 0;
    
    for (int i = 0; i < padding; i++) {
        printf(" ");
    }
    printf("%s\n", text);
}

static void print_line(char c, int length) {
    for (int i = 0; i < length; i++) {
        printf("%c", c);
    }
    printf("\n");
}

void ui_print_header(const char* title, const char* subtitle) {
    CLEAR_SCREEN();
    printf("\n");
    print_line('=', 60);
    print_centered_text(title, 60);
    if (subtitle) {
        print_centered_text(subtitle, 60);
    }
    print_line('=', 60);
    printf("\n");
}

void ui_print_menu(const menu* m) {
    if (!m) return;
    
    printf("\n");
    if (m->subtitle) {
        printf("  %s\n", m->subtitle);
        print_line('-', 60);
    }
    printf("\n");
    
    for (int i = 0; i < m->item_count; i++) {
        menu_item* item = &m->items[i];
        if (item) {
            printf("  [%d] %s", i + 1, item->title);
            if (item->description) {
                int spaces = 55 - (int)strlen(item->title);
                for (int j = 0; j < spaces && spaces > 0; j++) {
                    printf(" ");
                }
                printf("- %s", item->description);
            }
            printf("\n");
        }
    }
    
    printf("\n");
    print_line('=', 60);
    printf("\n");
}

void ui_print_divider(void) {
    print_line('-', 60);
}

void ui_print_success(const char* message) {
    printf("\n  [✓] %s\n\n", message);
}

void ui_print_error(const char* message) {
    printf("\n  [✗] Error: %s\n\n", message);
}

void ui_print_info(const char* message) {
    printf("\n  [i] %s\n\n", message);
}

static char* trim_whitespace(char* str) {
    if (!str) return str;
    
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == 0) return str;
    
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = 0;
    
    return str;
}

int ui_read_choice(int min_choice, int max_choice) {
    printf("  Enter your choice (%d-%d): ", min_choice, max_choice);
    
    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
        return -1;
    }
    
    trim_whitespace(input_buffer);
    
    if (strlen(input_buffer) == 0) {
        return -1;
    }
    
    char* endptr;
    long choice = strtol(input_buffer, &endptr, 10);
    
    if (*endptr != '\0' || choice < min_choice || choice > max_choice) {
        return -1;
    }
    
    return (int)choice;
}

char* ui_read_string(const char* prompt, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return NULL;
    
    printf("  %s: ", prompt);
    
    if (fgets(buffer, (int)buffer_size, stdin) != NULL) {
        trim_whitespace(buffer);
        return buffer;
    }
    
    buffer[0] = '\0';
    return NULL;
}

char* ui_read_path(const char* prompt, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return NULL;
    
    printf("  %s\n  (Press Enter to browse): ", prompt);
    
    if (fgets(buffer, (int)buffer_size, stdin) != NULL) {
        trim_whitespace(buffer);
        
        if (strlen(buffer) == 0) {
            return NULL;
        }
        
        return buffer;
    }
    
    buffer[0] = '\0';
    return NULL;
}

int ui_confirm(const char* message) {
    char response[8];
    
    printf("\n  %s (y/n): ", message);
    
    if (fgets(response, sizeof(response), stdin) != NULL) {
        trim_whitespace(response);
        
        if (strlen(response) > 0 && (response[0] == 'y' || response[0] == 'Y')) {
            return 1;
        }
    }
    
    return 0;
}

void ui_pause(void) {
    printf("\n  Press Enter to continue...");
    GETCH();
}

void ui_clear_screen(void) {
    CLEAR_SCREEN();
}

#ifdef _WIN32
static int list_files_in_directory(const char* dir_path, char*** files, int* count) {
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    char search_path[MAX_PATH_LEN];
    int capacity = 100;
    int cnt = 0;
    char** file_list = (char**)malloc(sizeof(char*) * capacity);
    
    if (!file_list) return 0;
    
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);
    
    hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        free(file_list);
        return 0;
    }
    
    do {
        if (strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0) {
            if (cnt >= capacity) {
                capacity *= 2;
                char** new_list = (char**)realloc(file_list, sizeof(char*) * capacity);
                if (!new_list) {
                    for (int i = 0; i < cnt; i++) free(file_list[i]);
                    free(file_list);
                    FindClose(hFind);
                    return 0;
                }
                file_list = new_list;
            }
            
            file_list[cnt] = (char*)malloc(MAX_PATH_LEN);
            if (file_list[cnt]) {
                strncpy(file_list[cnt], find_data.cFileName, MAX_PATH - 1);
                file_list[cnt][MAX_PATH - 1] = '\0';
                cnt++;
            }
        }
    } while (FindNextFile(hFind, &find_data));
    
    FindClose(hFind);
    
    *files = file_list;
    *count = cnt;
    
    return 1;
}
#else
static int list_files_in_directory(const char* dir_path, char*** files, int* count) {
    DIR* dir;
    struct dirent* entry;
    int capacity = 100;
    int cnt = 0;
    char** file_list = (char**)malloc(sizeof(char*) * capacity);
    
    if (!file_list) return 0;
    
    dir = opendir(dir_path);
    if (!dir) {
        free(file_list);
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (cnt >= capacity) {
                capacity *= 2;
                char** new_list = (char**)realloc(file_list, sizeof(char*) * capacity);
                if (!new_list) {
                    for (int i = 0; i < cnt; i++) free(file_list[i]);
                    free(file_list);
                    closedir(dir);
                    return 0;
                }
                file_list = new_list;
            }
            
            file_list[cnt] = (char*)malloc(MAX_PATH_LEN);
            if (file_list[cnt]) {
                strncpy(file_list[cnt], entry->d_name, MAX_PATH_LEN - 1);
                file_list[cnt][MAX_PATH_LEN - 1] = '\0';
                cnt++;
            }
        }
    }
    
    closedir(dir);
    
    *files = file_list;
    *count = cnt;
    
    return 1;
}
#endif

static int browse_for_file(char* selected_path, size_t path_size, const char* extensions[], int ext_count) {
    char current_dir[MAX_PATH_LEN];
    char** files = NULL;
    int file_count = 0;
    int choice;
    
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        strncpy(current_dir, ".", sizeof(current_dir) - 1);
    }
    
    while (1) {
        CLEAR_SCREEN();
        printf("\n");
        printf("  ╔══════════════════════════════════════════════════╗\n");
        printf("  ║              Select DMG File                    ║\n");
        printf("  ╚══════════════════════════════════════════════════╝\n");
        printf("\n  Current Directory: %s\n\n", current_dir);
        
        if (!list_files_in_directory(current_dir, &files, &file_count)) {
            printf("  Error: Cannot read directory\n");
            ui_pause();
            return 0;
        }
        
        printf("  [0] .. (Go back)\n");
        printf("  [1] . (Select this directory)\n");
        
        int show_count = 0;
        for (int i = 0; i < file_count && show_count < 50; i++) {
            size_t dirlen = strlen(current_dir);
            size_t filelen = strlen(files[i]);
            
            if (dirlen + 1 + filelen + 1 > MAX_PATH_LEN) {
                continue;
            }
            
            char full_path[MAX_PATH_LEN];
            memcpy(full_path, current_dir, dirlen);
            full_path[dirlen] = '/';
            memcpy(full_path + dirlen + 1, files[i], filelen);
            full_path[dirlen + 1 + filelen] = '\0';
            
            int is_dir = 0;
#ifdef _WIN32
            DWORD attr = GetFileAttributes(full_path);
            is_dir = (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
            struct stat st;
            if (stat(full_path, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
            }
#endif
            
            int matches_extension = 0;
            if (!is_dir) {
                for (int e = 0; e < ext_count; e++) {
                    size_t name_len = strlen(files[i]);
                    size_t ext_len = strlen(extensions[e]);
                    if (name_len > ext_len && 
                        strcasecmp(files[i] + name_len - ext_len, extensions[e]) == 0) {
                        matches_extension = 1;
                        break;
                    }
                }
            }
            
            if (is_dir || matches_extension) {
                printf("  [%d] %s%s\n", show_count + 2, files[i], is_dir ? "/" : "");
                show_count++;
            }
        }
        
        printf("\n  Enter choice (0-%d): ", show_count + 1);
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');
        
        if (choice == 0) {
            char* last_slash = strrchr(current_dir, '/');
            if (!last_slash) {
#ifdef _WIN32
                last_slash = strrchr(current_dir, '\\');
#endif
            }
            if (last_slash && last_slash != current_dir) {
                *last_slash = '\0';
            } else {
#ifdef _WIN32
                snprintf(current_dir, sizeof(current_dir), "C:\\");
#else
                snprintf(current_dir, sizeof(current_dir), "/");
#endif
            }
        } else if (choice == 1) {
            strncpy(selected_path, current_dir, path_size - 1);
            selected_path[path_size - 1] = '\0';
            
            for (int i = 0; i < file_count; i++) free(files[i]);
            free(files);
            return 1;
        } else if (choice >= 2 && choice <= show_count + 1) {
            int file_index = choice - 2;
            int actual_index = -1;
            int current_idx = -1;
            
            for (int i = 0; i < file_count; i++) {
                size_t dirlen = strlen(current_dir);
                size_t filelen = strlen(files[i]);
                
                if (dirlen + 1 + filelen + 1 > MAX_PATH_LEN) {
                    continue;
                }
                
                char full_path[MAX_PATH_LEN];
                memcpy(full_path, current_dir, dirlen);
                full_path[dirlen] = '/';
                memcpy(full_path + dirlen + 1, files[i], filelen);
                full_path[dirlen + 1 + filelen] = '\0';
                
                int is_dir = 0;
#ifdef _WIN32
                DWORD attr = GetFileAttributes(full_path);
                is_dir = (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
                struct stat st;
                if (stat(full_path, &st) == 0) {
                    is_dir = S_ISDIR(st.st_mode);
                }
#endif
                
                int matches_extension = 0;
                if (!is_dir) {
                    for (int e = 0; e < ext_count; e++) {
                        size_t name_len = strlen(files[i]);
                        size_t ext_len = strlen(extensions[e]);
                        if (name_len > ext_len && 
                            strcasecmp(files[i] + name_len - ext_len, extensions[e]) == 0) {
                            matches_extension = 1;
                            break;
                        }
                    }
                }
                
                if (is_dir || matches_extension) {
                    current_idx++;
                    if (current_idx == file_index) {
                        actual_index = i;
                        break;
                    }
                }
            }
            
            if (actual_index >= 0) {
                size_t dirlen = strlen(current_dir);
                size_t filelen = strlen(files[actual_index]);
                
                if (dirlen + 1 + filelen + 1 <= MAX_PATH_LEN) {
                    char full_path[MAX_PATH_LEN];
                    memcpy(full_path, current_dir, dirlen);
                    full_path[dirlen] = '/';
                    memcpy(full_path + dirlen + 1, files[actual_index], filelen);
                    full_path[dirlen + 1 + filelen] = '\0';
                    
                    int is_dir = 0;
#ifdef _WIN32
                    DWORD attr = GetFileAttributes(full_path);
                    is_dir = (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
                    struct stat st;
                    if (stat(full_path, &st) == 0) {
                        is_dir = S_ISDIR(st.st_mode);
                    }
#endif
                    
                    if (is_dir) {
                        strncpy(current_dir, full_path, sizeof(current_dir) - 1);
                        current_dir[sizeof(current_dir) - 1] = '\0';
                    } else {
                        strncpy(selected_path, full_path, path_size - 1);
                        selected_path[path_size - 1] = '\0';
                        
                        for (int i = 0; i < file_count; i++) free(files[i]);
                        free(files);
                        return 1;
                    }
                }
            }
        }
        
        for (int i = 0; i < file_count; i++) free(files[i]);
        free(files);
        files = NULL;
        file_count = 0;
    }
}

static int handle_install_dmg(void) {
    char dmg_path[MAX_PATH_LEN];
    char extensions[][8] = {".dmg", ".iso", ".img"};
    
    ui_print_header("Install from DMG", "Step 1: Select DMG File");
    
    printf("  This wizard will help you install an application from a DMG file.\n\n");
    
    if (!browse_for_file(dmg_path, sizeof(dmg_path), (const char**)extensions, 3)) {
        ui_print_error("No file selected");
        ui_pause();
        return 0;
    }
    
    ui_print_header("Install from DMG", "Step 2: Installation Progress");
    printf("  Selected file: %s\n\n", dmg_path);
    
    printf("  Opening DMG file...\n");
    dmg_context* ctx = NULL;
    
    if (!dmg_open(dmg_path, &ctx)) {
        ui_print_error("Failed to open DMG file");
        ui_pause();
        return 0;
    }
    
    printf("\n  DMG Information:\n");
    ui_print_divider();
    dmg_print_info(ctx);
    ui_print_divider();
    
    printf("\n  Extracting application...\n");
    
    if (ctx->is_raw_hfs) {
        printf("  Detected raw HFS+ image\n");
        dmg_extract_from_raw_hfs(ctx, "./extracted");
    } else {
        printf("  Searching for .app bundle in DMG...\n");
        dmg_find_and_extract_app(ctx, "*.app", "./extracted");
    }
    
    dmg_close(ctx);
    
    ui_print_success("Extraction completed!");
    printf("\n  The application has been extracted to: ./extracted\n");
    
    ui_pause();
    return 1;
}

static int handle_list_apps(void) {
    ui_print_header("Installed Applications", NULL);
    
    if (!app_manager_init(NULL)) {
        ui_print_error("Failed to initialize app manager");
        ui_pause();
        return 0;
    }
    
    app_bundle_list_installed();
    
    app_manager_cleanup();
    
    ui_pause();
    return 1;
}

static int handle_uninstall_app(void) {
    char bundle_id[MAX_LINE_LENGTH];
    
    ui_print_header("Uninstall Application", NULL);
    
    if (!app_manager_init(NULL)) {
        ui_print_error("Failed to initialize app manager");
        ui_pause();
        return 0;
    }
    
    printf("  Enter the Bundle ID of the application to uninstall.\n");
    printf("  (Use 'list' command to see installed applications)\n\n");
    
    if (!ui_read_string("Bundle ID", bundle_id, sizeof(bundle_id))) {
        app_manager_cleanup();
        return 0;
    }
    
    if (strlen(bundle_id) == 0) {
        ui_print_error("Bundle ID cannot be empty");
        app_manager_cleanup();
        ui_pause();
        return 0;
    }
    
    if (!ui_confirm("Are you sure you want to uninstall this application?")) {
        ui_print_info("Uninstall cancelled");
        app_manager_cleanup();
        return 0;
    }
    
    printf("\n  Uninstalling application...\n");
    
    if (app_bundle_uninstall(bundle_id)) {
        ui_print_success("Application uninstalled successfully!");
    } else {
        ui_print_error("Failed to uninstall application");
    }
    
    app_manager_cleanup();
    ui_pause();
    return 1;
}

static int handle_dmg_info(void) {
    char dmg_path[MAX_PATH_LEN];
    char extensions[][8] = {".dmg", ".iso", ".img"};
    
    ui_print_header("DMG File Information", "Select a DMG file to view details");
    
    if (!browse_for_file(dmg_path, sizeof(dmg_path), (const char**)extensions, 3)) {
        ui_print_error("No file selected");
        ui_pause();
        return 0;
    }
    
    ui_print_header("DMG File Information", dmg_path);
    
    dmg_context* ctx = NULL;
    
    if (!dmg_open(dmg_path, &ctx)) {
        ui_print_error("Failed to open DMG file");
        ui_pause();
        return 0;
    }
    
    printf("\n");
    dmg_print_info(ctx);
    
    dmg_close(ctx);
    
    ui_pause();
    return 1;
}

static int handle_test_macho(void) {
    char macho_path[MAX_PATH_LEN];
    char extensions[][8] = {".app", ".dylib", ".bundle", ""};
    
    ui_print_header("Test Mach-O File", "Select a Mach-O executable to test");
    
    if (!browse_for_file(macho_path, sizeof(macho_path), (const char**)extensions, 4)) {
        ui_print_error("No file selected");
        ui_pause();
        return 0;
    }
    
    ui_print_header("Test Mach-O File", macho_path);
    
    printf("  Testing Mach-O load: %s\n", macho_path);
    ui_print_divider();
    
    init_syscall_table();
    vfs_init();
    dyld_init();
    
    struct mach_header* header = NULL;
    bool is_64bit = false;
    
    if (!parse_macho_header(macho_path, &header, &is_64bit)) {
        ui_print_error("Failed to parse Mach-O header");
        dyld_cleanup();
        vfs_cleanup();
        ui_pause();
        return 0;
    }
    
    printf("\n  Mach-O Header Info:\n");
    printf("    Magic:          0x%x\n", header->magic);
    printf("    CPU Type:       0x%x\n", header->cputype);
    printf("    File Type:      0x%x\n", header->filetype);
    printf("    Number of cmds: %d\n", header->ncmds);
    printf("    Size of cmds:   %d\n", header->sizeofcmds);
    printf("    64-bit:         %s\n", is_64bit ? "Yes" : "No");
    
    void* base_address = NULL;
    if (!map_macho_segments(macho_path, header, is_64bit, &base_address)) {
        ui_print_error("Failed to map Mach-O segments");
        free(header);
        dyld_cleanup();
        vfs_cleanup();
        ui_pause();
        return 0;
    }
    
    printf("\n  Segment Mapping:\n");
    printf("    Base address:   %p\n", base_address);
    
    void* main_addr = find_main_function(macho_path, header, is_64bit, base_address);
    printf("\n  Main Function:\n");
    if (main_addr) {
        printf("    Address:        %p\n", main_addr);
        printf("    Status:         Found\n");
    } else {
        printf("    Status:         Not found\n");
    }
    
    unmap_macho_segments(base_address, header, is_64bit);
    free(header);
    dyld_cleanup();
    vfs_cleanup();
    
    ui_print_success("Test completed successfully!");
    ui_pause();
    return 1;
}

int ui_run_interactive_mode(void) {
    menu main_menu;
    menu_item items[MAX_MENU_ITEMS];
    
    items[0] = (menu_item){"Install from DMG", "Install an application from a DMG file", "dmg-install", 0, NULL};
    items[1] = (menu_item){"List Applications", "Show all installed applications", "list", 0, NULL};
    items[2] = (menu_item){"Uninstall Application", "Remove an installed application", "uninstall", 0, NULL};
    items[3] = (menu_item){"DMG Information", "View details about a DMG file", "dmg-info", 0, NULL};
    items[4] = (menu_item){"Test Mach-O File", "Test loading a Mach-O executable", "test", 0, NULL};
    items[5] = (menu_item){"Exit", "Exit the application", "exit", 0, NULL};
    
    main_menu.title = "WinDarling Application Manager";
    main_menu.subtitle = "Welcome! Choose an option to get started.";
    main_menu.item_count = 6;
    main_menu.items = items;
    
    while (1) {
        ui_print_menu(&main_menu);
        
        int choice = ui_read_choice(1, main_menu.item_count);
        
        if (choice < 0) {
            ui_print_error("Invalid input. Please enter a number.");
            continue;
        }
        
        switch (choice) {
            case 1:
                handle_install_dmg();
                break;
            case 2:
                handle_list_apps();
                break;
            case 3:
                handle_uninstall_app();
                break;
            case 4:
                handle_dmg_info();
                break;
            case 5:
                handle_test_macho();
                break;
            case 6:
                ui_print_header("Goodbye!", "Thank you for using WinDarling");
                printf("\n  Exiting...\n\n");
                return 0;
            default:
                ui_print_error("Invalid choice");
                break;
        }
    }
    
    return 0;
}
