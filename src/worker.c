#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "lock.h"
#include <sched.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

int main(const int argc, char *argv[]) {
    if (argc < 3) return 1;
    const char *target_file = argv[1];
    const char *stats_file = argv[2];
    const pid_t pid = getpid();

    signal(SIGINT, handle_sigint);

    int total_locks = 0;

    while (keep_running) {
        Lock *l = get_lock(target_file);

        if (!l) {
            printf("Some error while taking lock! %d PID\n", pid);
            return 1;
        }

        total_locks++;
        sleep(1);
        if (unlock(l) != 0) {
            printf("Some error while unlocking lock! %d PID\n", pid);
            return 1;
        }
        // Небольшая пауза, чтобы другие успели взять лок
        sched_yield();
    }

    FILE *f = fopen(stats_file, "a");
    if (f) {
        fprintf(f, "PID %d: %d locks\n", pid, total_locks);
        fclose(f);
    }

    return 0;
}
