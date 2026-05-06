#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

static FILE *log_out = NULL;
static int log_fd = -1;

void logger_init(FILE *output) {
    log_out = output;
    log_fd = fileno(output);
}

int get_log_file_fd() {
    return log_fd;
}

static void print_log(const char *level, const char *fmt, va_list args) {
    if (log_out == NULL)
        return;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm tm_info;
    localtime_r(&tv.tv_sec, &tm_info);

    fprintf(
        log_out,
        "[%02d:%02d:%02d.%03ld] [%s] ",
        tm_info.tm_hour,
        tm_info.tm_min,
        tm_info.tm_sec,
        tv.tv_usec / 1000,
        level
    );

    vfprintf(log_out, fmt, args);

    fprintf(log_out, "\n");
    fflush(log_out);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log("INFO", fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log("WARN", fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print_log("ERROR", fmt, args);
    va_end(args);
}

void log_perror(const char *fmt, ...) {
    char user_msg[1024];
    const int old_errno = errno;

    va_list args;
    va_start(args, fmt);
    vsnprintf(user_msg, sizeof(user_msg), fmt, args);
    va_end(args);

    log_error("%s: %s", user_msg, strerror(old_errno));
}

void log_pwarn(const char *fmt, ...) {
    char user_msg[1024];
    const int old_errno = errno;

    va_list args;
    va_start(args, fmt);
    vsnprintf(user_msg, sizeof(user_msg), fmt, args);
    va_end(args);

    log_warn("%s: %s", user_msg, strerror(old_errno));
}