#include "logger.h"

#include <stdio.h>
#include <unistd.h>

extern int run_server(const char *config_file_path);

int main(const int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./server <config_file> [log_file]\n");
        return 1;
    }

    const char *config_file_path = argv[1];
    const char *log_file_path = "server.log";
    if (argc == 3) {
        log_file_path = argv[2];
    }

    FILE *log_file = fopen(log_file_path, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return 1;
    }

    logger_init(log_file);

    const int res = run_server(config_file_path);

    if (fclose(log_file) != 0) {
        perror("Failed to close log file");
        return 1;
    }

    return res;
}
