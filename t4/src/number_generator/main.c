#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(const int argc, char **argv) {
    char *data_file_path = "numbers.txt";
    if (argc == 2) {
        data_file_path = argv[1];
    }

    FILE *f = fopen(data_file_path, "w");
    if (!f) {
        perror("Failed to open data file");
        return 1;
    }

    int sum = 0;
    srand(time(NULL));

    for (int i = 0; i < 999; i++) {
        const int num = (rand() % 2000) - 1000;
        sum += num;
        fprintf(f, "%d\n", num);
    }

    fprintf(f, "%d\n", -sum);
    fclose(f);
    return 0;
}
