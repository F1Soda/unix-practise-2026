#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <errno.h>
#include <string.h>

// Инициализация: передаем файл или NULL
void logger_init(FILE *output);
int get_log_file_fd();

void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_perror(const char *fmt, ...);

#endif
