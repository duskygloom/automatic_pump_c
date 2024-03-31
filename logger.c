#include "logger.h"

#include <stdio.h>
#include <stdarg.h>

static level_t log_level = DEFAULT_LEVEL;

void set_log_level(level_t level)
{
    log_level = level;
}

void write_log(level_t level, const char *format, ...)
{
    if (level < log_level) return;
    switch (level) {
        case DEBUG:
            printf("[DEBUG]   ");
            break;
        case WARNING:
            printf("[WARNING] ");
            break;
        case INFO:
            printf("[INFO]    ");
            break;
        case ERROR:
            printf("[ERROR]   ");
            break;
    }
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    putchar('\n');
}
