#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#include "logger.h"

static void safe_uint_to_str(char *buf, unsigned long val) {
    char tmp[21];
    int i = 0;
    if (val == 0) { tmp[i++] = '0'; }
    else {
        for (; val > 0; val /= 10) tmp[i++] = (val % 10) + '0';
    }
    while (i > 0) *buf++ = tmp[--i];
    *buf = '\0';
}

static void crash_handler(int sig) {
    const int log_fd = get_log_file_fd();
    if (log_fd == -1) _exit(128 + sig);

    const char *sig_name;
    switch (sig) {
        case SIGSEGV: sig_name = "SIGSEGV"; break;
        case SIGABRT: sig_name = "SIGABRT"; break;
        case SIGFPE:  sig_name = "SIGFPE";  break;
        case SIGILL:  sig_name = "SIGILL";  break;
        case SIGBUS:  sig_name = "SIGBUS";  break;
        case SIGPIPE: sig_name = "SIGPIPE"; break;
        default:      sig_name = "UNKNOWN";
    }

    char buf[256];
    char num_buf[32];

    write(log_fd, "[FATAL] PID ", 13);

    safe_uint_to_str(num_buf, (unsigned long)getpid());
    write(log_fd, num_buf, strlen(num_buf));

    write(log_fd, " caught ", 8);
    write(log_fd, sig_name, strlen(sig_name));

    write(log_fd, ". Terminating...\n", 17);

    fsync(log_fd);

    _exit(128 + sig);
}

void install_crash_handlers() {
    struct sigaction sa;
    sa.sa_handler = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;

    const int signals[] = {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGPIPE, SIGSYS};
    for (size_t i = 0; i < sizeof(signals)/sizeof(signals[0]); i++) {
        sigaction(signals[i], &sa, NULL);
    }
}