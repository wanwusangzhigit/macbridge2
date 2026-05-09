#include "util.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ConfigItem {
    char* key;
    char* value;
    struct ConfigItem* next;
} ConfigItem;

static ConfigItem* config_head = NULL;
static LogLevel log_level = LOG_LEVEL_INFO;
static FILE* log_file = NULL;
static bool log_initialized = false;
static int progress_total = 0;
static int progress_current = 0;
static bool progress_active = false;

bool config_init(void) {
    config_set("install_dir", "./Applications");
    config_set("log_level", "info");
    config_set("color_output", "true");
    return true;
}

void config_cleanup(void) {
    ConfigItem* current = config_head;
    while (current) {
        ConfigItem* next = (ConfigItem*)current->next;
        free(current->key);
        free(current->value);
        free(current);
        current = next;
    }
    config_head = NULL;
}

const char* config_get(const char* key) {
    if (!key) return NULL;
    
    ConfigItem* current = config_head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = (ConfigItem*)current->next;
    }
    return NULL;
}

bool config_set(const char* key, const char* value) {
    if (!key) return false;
    
    ConfigItem* current = config_head;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            free(current->value);
            current->value = value ? strdup(value) : NULL;
            return true;
        }
        current = (ConfigItem*)current->next;
    }
    
    ConfigItem* item = (ConfigItem*)malloc(sizeof(ConfigItem));
    if (!item) return false;
    
    item->key = strdup(key);
    item->value = value ? strdup(value) : NULL;
    item->next = config_head;
    config_head = item;
    
    return true;
}

static void log_message(LogLevel level, const char* prefix, const char* format, va_list args) {
    if (level < log_level) return;
    
    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    if (log_file) {
        fprintf(log_file, "[%s] [%s] ", timestamp, prefix);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    const char* color = ANSI_RESET;
    if (level == LOG_LEVEL_WARNING) color = ANSI_YELLOW;
    if (level == LOG_LEVEL_ERROR) color = ANSI_RED;
    
    fprintf(stdout, "%s[%s] [%s] ", color, timestamp, prefix);
    vfprintf(stdout, format, args);
    fprintf(stdout, ANSI_RESET "\n");
}

bool log_init(const char* filename, LogLevel level) {
    log_level = level;
    log_file = NULL;
    
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            fprintf(stderr, "Warning: Could not open log file %s\n", filename);
        }
    }
    
    log_initialized = true;
    log_info("Log system initialized");
    return true;
}

void log_cleanup(void) {
    if (log_initialized) {
        log_info("Log system shutting down");
    }
    
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    log_initialized = false;
}

void log_set_level(LogLevel level) {
    log_level = level;
}

void log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_DEBUG, "DEBUG", format, args);
    va_end(args);
}

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_INFO, "INFO", format, args);
    va_end(args);
}

void log_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_WARNING, "WARNING", format, args);
    va_end(args);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_LEVEL_ERROR, "ERROR", format, args);
    va_end(args);
}

void progress_bar_init(int total) {
    progress_total = total;
    progress_current = 0;
    progress_active = true;
    fprintf(stdout, "\n");
    progress_bar_update(0);
}

void progress_bar_update(int current) {
    if (!progress_active) return;
    
    progress_current = current;
    float progress = (float)current / progress_total;
    int bar_width = 50;
    int pos = bar_width * progress;
    
    fprintf(stdout, "\r[");
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) fprintf(stdout, "=");
        else if (i == pos) fprintf(stdout, ">");
        else fprintf(stdout, " ");
    }
    fprintf(stdout, "] %d%% (%d/%d)", (int)(progress * 100), current, progress_total);
    fflush(stdout);
}

void progress_bar_finish(void) {
    if (!progress_active) return;
    
    progress_bar_update(progress_total);
    fprintf(stdout, "\n\n");
    progress_active = false;
}
