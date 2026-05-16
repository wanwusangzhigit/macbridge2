#ifndef UI_H
#define UI_H

#include <stdio.h>

typedef struct {
    const char* title;
    const char* description;
    const char* command;
    int argc;
    char** argv;
} menu_item;

typedef struct {
    const char* title;
    const char* subtitle;
    int item_count;
    menu_item* items;
} menu;

void ui_print_header(const char* title, const char* subtitle);
void ui_print_menu(const menu* m);
void ui_print_divider(void);
void ui_print_success(const char* message);
void ui_print_error(const char* message);
void ui_print_info(const char* message);
int ui_read_choice(int min_choice, int max_choice);
char* ui_read_string(const char* prompt, char* buffer, size_t buffer_size);
char* ui_read_path(const char* prompt, char* buffer, size_t buffer_size);
int ui_confirm(const char* message);
void ui_pause(void);
void ui_clear_screen(void);
int ui_run_interactive_mode(void);

#endif
