#ifndef T1_VALIDATION_H
#define T1_VALIDATION_H
#include "config_parser.h"

int validate_absolute_path(const char *path);

int validate_exec_config(ExecConfig *exec_config);

#endif //T1_VALIDATION_H