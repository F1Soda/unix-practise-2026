#include "logger.h"
#include "validation.h"

#include <syslog.h>
#include <unistd.h>

extern int demonize();
extern int run_myinit(const char *config_file_path);
extern void install_crash_handlers();

int main(const int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./sparser <config_file> [log_file]\n");
        return 1;
    }

    const char *config_file_path = argv[1];
    const char *log_file_path = "/tmp/myinit.log";
    if (argc == 3) {
        log_file_path = argv[2];
    }

    if (validate_absolute_path(config_file_path) != 0) {
        fprintf(stderr, "config_file: allowed only absolute path: %s\n", config_file_path);
        return 1;
    }

    if (validate_absolute_path(log_file_path) != 0) {
        fprintf(stderr, "log_file: allowed only absolute path: %s\n", log_file_path);
        return 1;
    }

    openlog("myinit", LOG_PID | LOG_CONS, LOG_DAEMON);

    if (demonize() != 0) {
        perror("demonize");
        return 1;
    }

    FILE *log_file = fopen(log_file_path, "a");
    if (log_file == NULL) {
        syslog(LOG_ERR, "Could not open log file %s: %m", log_file_path);
        closelog();
        return 1;
    }

    logger_init(log_file);
    install_crash_handlers();
    int res = run_myinit(config_file_path);

    if (fclose(log_file) != 0) {
        syslog(LOG_ERR, "Could not close log file %s: %m", log_file_path);
        res = 1;
    }

    closelog();
    return res;
}
