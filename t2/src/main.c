#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "lock.h"

int do_something_with_file(const char *filename) {
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("do_something_with_file: fopen");
        return 1;
    }

    sleep(1);

    const time_t now = time(NULL);
    const struct tm *local = localtime(&now);
    const pid_t pid = getpid();

    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local);

    fprintf(file, "[PID: %d] Time: %s\n", pid, time_str);

    if (fclose(file) != 0) {
        perror("do_something_with_file: fclose");
        return 1;
    }

    return 0;
}


int main(const int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./lock <file>\n");
        return 1;
    }

    const char *filename = argv[1];

    Lock *lock = get_lock(filename);
    if (lock == NULL) return 1;

    const int res = do_something_with_file(filename);

    if (res != 0) {
        unlock(lock);
        return 1;
    }

    if (unlock(lock) != 0) {
        return 1;
    }

    return 0;
}