#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE
} LogLevel;

bool config_init(void);
void config_cleanup(void);
const char* config_get(const char* key);
bool config_set(const char* key, const char* value);

bool log_init(const char* log_file, LogLevel level);
void log_cleanup(void);
void log_set_level(LogLevel level);
void log_debug(const char* format, ...);
void log_info(const char* format, ...);
void log_warning(const char* format, ...);
void log_error(const char* format, ...);

#define ANSI_RESET   "\033[0m"
#define ANSI_BLACK   "\033[30m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_BOLD    "\033[1m"

void progress_bar_init(int total);
void progress_bar_update(int current);
void progress_bar_finish(void);

#endif
