#include <string.h>
#include <unistd.h>

#include "config_parser.h"
#include "logger.h"

int validate_absolute_path(const char *path) {
    const int len = strlen(path);
    if (len == 0 || path[0] != '/') {
        return 1;
    }
    return 0;
}

int validate_exec_config(const ExecConfig *exec_config) {
    if (validate_absolute_path(exec_config->exec_path) != 0) {
        log_warn("Validation: allowed only absolute path: %s",
                 exec_config->exec_path,
                 exec_config->exec_path);
        return 1;
    }
    if (validate_absolute_path(exec_config->in_file_path) != 0) {
        log_warn("parsed config: allowed only absolute path: %s",
                 exec_config->in_file_path,
                 exec_config->exec_path);
        return 1;
    }
    if (validate_absolute_path(exec_config->out_file_path) != 0) {
        log_warn("parsed config: allowed only absolute path: %s",
                 exec_config->out_file_path,
                 exec_config->exec_path);
        return 1;
    }

    if (access(exec_config->in_file_path, F_OK) != 0) {
        log_warn("Validation: Input file %s does not exist", exec_config->in_file_path);
        return 1;
    }

    if (access(exec_config->in_file_path, R_OK) != 0) {
        log_warn("Validation: Input file %s is not readable (permission denied)", exec_config->in_file_path);
        return 1;
    }


    if (access(exec_config->out_file_path, F_OK) == 0) {
        if (access(exec_config->out_file_path, W_OK) != 0) {
            log_warn("Validation: Output file %s is not writable", exec_config->out_file_path);
            return 1;
        }
    } else {
        log_info("Output file %s will be created", exec_config->out_file_path);
    }

    return 0;
}
