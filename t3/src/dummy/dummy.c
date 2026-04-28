#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    char *label = (argc > 1) ? argv[1] : "Default";

    srand(time(NULL) + getpid());

    while (1) {
        fprintf(stdout, "[%s] PID %d: %d\n", label, getpid(), rand() % 100);
        fflush(stdout);
        sleep(1);
    }

    return 0;
}
