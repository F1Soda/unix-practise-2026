#include "config_parser.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "validation.h"

#define MAX_LINE_LENGTH 1024
#define MAX_ARGS 8

ConfigList *parse(FILE *config_file) {
    ConfigList *result = calloc(1, sizeof(ConfigList));
    if (result == NULL) {
        log_perror("calloc: ConfigList *result");
        return NULL;
    }

    char line[MAX_LINE_LENGTH];
    int exec_count = 0;
    int i = -1;
    while (fgets(line, sizeof(line), config_file)) {
        i++;
        if (exec_count >= MAX_EXEC) {
            break;
        }

        line[strcspn(line, "\n")] = 0;
        if (line[0] == '\0' || line[0] == '#') continue;

        char *path = strtok(line, " ");
        if (path == NULL) {
            log_warn("Parse: failed to parse $d line: empty exec path", i);
            continue;
        }

        // + 2 для stdin и stdout параметров
        char *args_tmp[MAX_ARGS + 2];
        int argc = 0;
        char *token = strtok(NULL, " ");

        while (token != NULL) {
            if (argc >= MAX_ARGS + 2) {
                log_warn("Parse: too many arguments for %d line. Max allowed %d", i, MAX_ARGS);
                break;
            }

            args_tmp[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (argc < 2) {
            log_error("Parse: line %d for %s is too short (missing in/out files)", i, path);
            continue;
        }

        ExecConfig config;
        config.exec_path = strdup(path);
        config.in_file_path = strdup(args_tmp[argc - 2]);
        config.out_file_path = strdup(args_tmp[argc - 1]);

        config.argc = argc - 2 + 1; // +1 для самого пути в argv[0]
        config.argv = malloc((config.argc + 1) * sizeof(char *));

        config.argv[0] = strdup(path);
        for (int j = 0; j < argc - 2; j++) {
            config.argv[j + 1] = strdup(args_tmp[j]);
        }
        config.argv[config.argc] = NULL;

        if (validate_exec_config(&config) != 0) {
            log_error("Parse: invalid exec config on %d line. Skipping", i);
            continue;
        }

        result->items[exec_count] = config;
        exec_count++;
    }

    result->count = exec_count;
    return result;
}


void free_config_list(ConfigList *list) {
    if (list == NULL) return;

    for (int i = 0; i < list->count; i++) {
        free(list->items[i].exec_path);
        free(list->items[i].in_file_path);
        free(list->items[i].out_file_path);

        if (list->items[i].argv != NULL) {
            for (int j = 0; j < list->items[i].argc; j++) {
                free(list->items[i].argv[j]);
            }

            free(list->items[i].argv);
        }
    }

    free(list);
}