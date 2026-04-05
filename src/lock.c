#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include "lock.h"

#include <errno.h>
#include <libgen.h>
#include <asm-generic/errno-base.h>

#define LOG_INFO 0

char *get_lock_filename(const char *filename) {
    const char *lock_suffix = ".lck";
    const size_t lock_filename_len = strlen(filename) + strlen(lock_suffix) + 1;
    char *lock_filename = malloc(lock_filename_len);

    if (lock_filename == NULL) {
        perror("get_lock_filename: malloc");
        return NULL;
    }

    sprintf(lock_filename, "%s%s", filename, lock_suffix);

    return lock_filename;
}

Lock *get_lock(const char *filename) {
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "get_lock: file \"%s\" does not exist\n", filename);
        return NULL;
    }

    char *lock_filename = get_lock_filename(filename);
    if (lock_filename == NULL) return NULL;

    char *path_copy = strdup(filename);
    const char *dir_name = dirname(path_copy);

    const int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        free(lock_filename);
        perror("get_lock: inotify_init");
        return NULL;
    }

    const int wd = inotify_add_watch(inotify_fd, dir_name, IN_DELETE);
    if (wd == -1) {
        perror("get_lock: inotify_add_watch");
        free(path_copy);
        return NULL;
    }
    free(path_copy);

    int lock_fd;
    char buffer[1024];
    while (1) {
        lock_fd = open(lock_filename, O_RDWR | O_CREAT | O_EXCL, 0666);

        if (lock_fd != -1) break;

        if (errno != EEXIST) {
            perror("get_lock: open");
            goto error_cleanup;
        }

        const ssize_t len = read(inotify_fd, buffer, sizeof(buffer));

        if (len == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("get_lock: read inotify");
            goto error_cleanup;
        }

        if (len == 0) {
            fprintf(stderr, "get_lock: inotify stream closed\n");
            goto error_cleanup;
        }
    }

    if (inotify_rm_watch(inotify_fd, wd)) {
        perror("get_lock: inotify_rm_watch");
        close(inotify_fd);
        close(lock_fd);
        free(lock_filename);
        return NULL;
    }

    if (close(inotify_fd) != 0) {
        perror("get_lock: close fd_inotify");
        close(lock_fd);
        free(lock_filename);
        return NULL;
    }

    const pid_t pid = getpid();
    if (dprintf(lock_fd, "%d\n", pid) < 0) {
        fprintf(stderr, "get_lock: failed to write pid to lock file\n");
        close(lock_fd);
        free(lock_filename);
        return NULL;
    }

    Lock *res = malloc(sizeof(Lock));
    if (!res) {
        perror("get_lock: malloc");
        close(lock_fd);
        free(lock_filename);
        return NULL;
    }

    res->fd = lock_fd;
    res->name = lock_filename;

    if (LOG_INFO)
        printf("Created lock file \"%s\" by %d PID\n", lock_filename, pid);

    return res;

error_cleanup:
    inotify_rm_watch(inotify_fd, wd);
    close(inotify_fd);
    close(lock_fd);
    free(lock_filename);
    return NULL;
}

int unlock(Lock *lock) {
    if (!lock) {
        fprintf(stderr, "unlock: null lock\n");
        return -1;
    }

    if (!lock->name) {
        fprintf(stderr, "unlock: null name\n");
        return -1;
    }

    if (lseek(lock->fd, 0, SEEK_SET)) {
        perror("unlock: lseek");
        return 1;
    }

    char data[32];
    if (read(lock->fd, data, 32) < 0) {
        fprintf(stderr, "unlock: failed to read PID in lock fd \"%d\"\n", lock->fd);
        return 1;
    }

    int parsed_pid;
    if (sscanf(data, "%d", &parsed_pid) != 1) {
        fprintf(stderr, "unlock: failed to parse \"%s\" to PID in lock file\n", data);
        return 1;
    }

    const pid_t expected_pid = getpid();
    if (parsed_pid != expected_pid) {
        fprintf(stderr, "unlock: expected %d PID in lock file but was %d PID\n", expected_pid, parsed_pid);
        return 1;
    }

    if (unlink(lock->name) != 0) {
        perror("unlock: unlink lock->name");
        return 1;
    }

    if (close(lock->fd) != 0) {
        perror("unlock: close lock->fd");
        return 1;
    }

    if (LOG_INFO)
        printf("Removed lock file \"%s\" by %d PID\n", lock->name, expected_pid);

    free(lock->name);
    free(lock);

    return 0;
}
