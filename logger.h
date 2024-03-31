#ifndef LOGGER_H
#define LOGGER_H

typedef enum { DEBUG, WARNING, INFO, ERROR } level_t;

#define DEFAULT_LEVEL   WARNING

void set_log_level(level_t level);
void write_log(level_t level, const char *format, ...);

#endif // LOGGER_H
