#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/fcntl-linux.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config_parser.h"
#include "logger.h"

static volatile sig_atomic_t reload_config_flag = 0;
static volatile sig_atomic_t shutdown_flag = 0;

static void handle_sighup(int _) {
    reload_config_flag = 1;
}

void handle_shutdown(int sig) {
    shutdown_flag = 1;
}

typedef struct {
    pid_t pid;
    int config_index;
} PidAndConfigIndex;

typedef struct {
    PidAndConfigIndex *items;
    int count;
} PidAndConfigIndexArray;

static void free_pid_and_config_index_array(PidAndConfigIndexArray *array) {
    if (array == NULL) return;

    if (array->items != NULL) {
        free(array->items);
    }

    free(array);
}

static ConfigList *get_config_list(const char *config_file_path) {
    FILE *config_file = fopen(config_file_path, "r");
    if (config_file == NULL) {
        log_perror("fopen config_file");
        return NULL;
    }

    ConfigList *config_list = parse(config_file);
    if (config_list == NULL) {
        log_error("Failed to parse config file");
        if (fclose(config_file) != 0) {
            log_perror("fclose");
        }
        return NULL;
    }

    if (fclose(config_file) != 0) {
        log_perror("fclose config_file");
        free_config_list(config_list);
        return NULL;
    }

    return config_list;
}

static int start_child_process(const ExecConfig *exec_config) {
    const pid_t cpid = fork();

    if (cpid == -1) {
        log_perror("fork");
        return -1;
    }

    if (cpid == 0) {
        int log_fd = get_log_file_fd();
        if (log_fd >= 0) {
            dup2(log_fd, STDERR_FILENO);
        }

        if (exec_config->in_file_path != NULL) {
            const int in_fd = open(exec_config->in_file_path, O_RDONLY);
            if (in_fd < 0) {
                log_perror("Failed open in_file_path for %s", exec_config->in_file_path);
                _exit(1);
            }
            dup2(in_fd, STDIN_FILENO);
            if (in_fd != STDIN_FILENO) {
                close(in_fd);
            }
        }

        if (exec_config->out_file_path != NULL) {
            const int out_fd = open(exec_config->out_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) {
                log_perror("Failed open out_file_path for %s", exec_config->out_file_path);
                _exit(1);
            }
            dup2(out_fd, STDOUT_FILENO);
            if (out_fd != STDOUT_FILENO) {
                close(out_fd);
            }
        }

        execvp(exec_config->exec_path, exec_config->argv);
        log_perror("Failed to exec %s", exec_config->exec_path);
        _exit(1);
    }

    return cpid;
}

static int start_children_processes(
    const ConfigList *config_list,
    PidAndConfigIndexArray *pid_and_config_index_array
) {
    pid_and_config_index_array->items = malloc(config_list->count * sizeof(PidAndConfigIndex));
    if (pid_and_config_index_array->items == NULL) {
        log_perror("calloc: result->items");
        free(pid_and_config_index_array);
        return 1;
    }

    int count_started_tasks = 0;
    for (int i = 0; i < config_list->count; i++) {
        const ExecConfig *exec_config = &config_list->items[i];

        PidAndConfigIndex pid_and_config;

        const int child_pid = start_child_process(exec_config);
        if (child_pid == -1) {
            log_warn("Occurred error while starting %d child: %s. Skipping", i, exec_config->exec_path);
            continue;
        }

        pid_and_config.pid = child_pid;
        pid_and_config.config_index = i;

        pid_and_config_index_array->items[count_started_tasks] = pid_and_config;
        count_started_tasks++;
        log_info("Process started: %s (pid %d)", exec_config->exec_path, child_pid);
    }

    pid_and_config_index_array->count = count_started_tasks;

    return 0;
}

static int stop_children_processes(
    const PidAndConfigIndexArray *pid_and_config_index_array,
    const ConfigList *config_list
) {
    int success_count = 0;

    for (int i = 0; i < pid_and_config_index_array->count; i++) {
        const pid_t current_pid = pid_and_config_index_array->items[i].pid;
        const int config_index = pid_and_config_index_array->items[i].config_index;
        const char *name = config_list->items[config_index].exec_path;

        if (kill(current_pid, SIGTERM) == 0) {
            success_count++;
            log_info("Process stopped: %s (pid %d)", name, current_pid);
        } else {
            log_warn("Failed to stop PID %d", current_pid);
        }
    }

    return success_count;
}

static int handle_child_death(
    pid_t cpid,
    const int status,
    const ConfigList *config_list,
    const PidAndConfigIndexArray *pid_and_config_index_array,
    int *running_processes
) {
    for (int p = 0; p < pid_and_config_index_array->count; p++) {
        if (pid_and_config_index_array->items[p].pid == cpid) {
            const PidAndConfigIndex *stopped_process = &pid_and_config_index_array->items[p];
            const int config_index = stopped_process->config_index;
            const ExecConfig *exec_config = &config_list->items[config_index];
            const char *exec_path = exec_config->exec_path;

            if (WIFEXITED(status)) {
                const int exit_code = WEXITSTATUS(status);
                log_info("Child %d (%s) finished with exit code: %d", cpid, exec_path, exit_code);
            } else if (WIFSIGNALED(status)) {
                const int sig_num = WTERMSIG(status);
                log_warn("Child %d (%s) terminated by signal: %d", cpid, exec_path, sig_num);
            } else {
                log_info("Child %d (%s) pid finished", cpid, exec_path);
            }

            const int new_child_pid = start_child_process(exec_config);
            if (new_child_pid == -1) {
                log_warn("Failed to restart child %d process: %s", cpid, exec_path);
                *running_processes = *running_processes - 1;
            } else {
                log_info("Successfully restarted child %d process %s", new_child_pid, exec_path);
                pid_and_config_index_array->items[p].pid = new_child_pid;
            }
        }
    }

    return 0;
}

static int read_config_and_start_children(
    const char *config_file_path,
    ConfigList **config_list,
    PidAndConfigIndexArray **pid_and_config_index_array
) {
    *config_list = get_config_list(config_file_path);
    if (*config_list == NULL) {
        log_error("Error reading config file");
        return 1;
    }

    *pid_and_config_index_array = calloc(1, sizeof(PidAndConfigIndexArray));
    if (*pid_and_config_index_array == NULL) {
        log_perror("calloc: PidAndConfigArray *result");
        return 1;
    }

    if (start_children_processes(*config_list, *pid_and_config_index_array) != 0) {
        log_error("Error starting children processes");
        free_config_list(*config_list);
        return 1;
    }

    return 0;
}

static int reload_config(
    const char *config_file_path,
    ConfigList **config_list,
    PidAndConfigIndexArray **pid_and_config_index_array
) {
    stop_children_processes(*pid_and_config_index_array, *config_list);

    free_config_list(*config_list);
    free_pid_and_config_index_array(*pid_and_config_index_array);

    while (waitpid(-1, NULL, WNOHANG) > 0) {
    }

    return read_config_and_start_children(config_file_path, config_list, pid_and_config_index_array);
}


int run_myinit(const char *config_file_path) {
    log_info("myinit started, PID: %d", getpid());

    ConfigList *config_list = NULL;
    PidAndConfigIndexArray *pid_and_config_index_array = NULL;

    if (read_config_and_start_children(config_file_path, &config_list, &pid_and_config_index_array) != 0) {
        return 1;
    }

    signal(SIGHUP, handle_sighup);
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    int running_processes = pid_and_config_index_array->count;
    while (!shutdown_flag && (running_processes > 0 || reload_config_flag)) {
        if (reload_config_flag) {
            log_info("SIGHUP received. Reloading configuration...");
            reload_config_flag = 0;

            if (reload_config(config_file_path, &config_list, &pid_and_config_index_array) != 0) {
                return 1;
            }
        }

        int status;
        const pid_t cpid = waitpid(-1, &status, WNOHANG);

        if (cpid == 0) {
            usleep(100000); // 100 мс
            continue;
        }

        if (cpid == -1) {
            if (errno == ECHILD) {
                log_info("No more children to watch. Exiting loop.");
                free_config_list(config_list);
                free_pid_and_config_index_array(pid_and_config_index_array);
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            perror("waitpid");
            free_config_list(config_list);
            free_pid_and_config_index_array(pid_and_config_index_array);
            break;
        }

        handle_child_death(cpid, status, config_list, pid_and_config_index_array, &running_processes);
    }

    if (shutdown_flag) {
        log_info("Shutdown signal received. Stopping all children...");
        stop_children_processes(pid_and_config_index_array, config_list);
    }

    log_info("Successfully stopping myinit: nothing to run");
    return 0;
}
