#ifndef T1_CONFIG_PARSER_H
#define T1_CONFIG_PARSER_H
#include <stdio.h>

#define MAX_EXEC 64

typedef struct {
    char *exec_path;
    char *in_file_path;
    char *out_file_path;
    int argc;
    char **argv;
} ExecConfig;

typedef struct {
    ExecConfig items[MAX_EXEC];
    int count;
} ConfigList;

ConfigList *parse(FILE* config_file);

void free_config_list(ConfigList *list);

#endif //T1_CONFIG_PARSER_H