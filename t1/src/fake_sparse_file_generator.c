#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"

#define DEFAULT_FILE_LENGTH (4 * 1024 * 1024 + 1)
// Вероятность, что блок в алгоритме будет содержать данные. По умолчанию вероятность 0.2
#define DEFAULT_DATA_BLOCK_PROBABILITY (0.2 * 100)

#define MIN(a,b) ((a) < (b) ? (a) : (b))

void fill_buffer(char* buffer, const int from, const int to, const char val) {
    for (int i = from; i < to; i++) {
        buffer[i] = val;
    }
}

int generate_fake_sparse_file(const int fd, const int buffer_size, char* buffer) {
    srandom(time(NULL));

    // Алгоритм следующий:
    // В каждом блоке случайно определяем, будет ли в нем находится данные:
    // если да и до этого прошлый блок тоже был с данными, то полностью заполняем блок данными;
    // если да и до этого прошлый блок был пустым, то случайно определяем индекс с которого в блоке начнем писать данные;
    // если нет и до этого прошлый блок был с данными, то случайно определяем индекс, где данные закончатся в этом блоке;
    // если нет и до этого прошлый блок был пустым, то полностью заполняем нулями;
    int is_data_span_now = random() % 2;
    int data_block_index = 0;

    const size_t remainder = DEFAULT_FILE_LENGTH % buffer_size;
    for (int i = 0; i < DEFAULT_FILE_LENGTH - remainder; i += buffer_size) {

        const int is_data_block = random() % 100 < DEFAULT_DATA_BLOCK_PROBABILITY;

        if (is_data_block) {
            const int from = is_data_span_now ? data_block_index : random() % buffer_size;
            const int to = buffer_size;

            fill_buffer(buffer, 0, from, 0);
            fill_buffer(buffer, from, to, 1);

            data_block_index = 0;
            is_data_span_now = 1;
        }
        else {
            if (is_data_span_now) {
                const int from = 0;
                const int to = random() % buffer_size;

                fill_buffer(buffer, from, to, 1);
                fill_buffer(buffer, to, buffer_size, 0);

                is_data_span_now = 0;
            }
            else {
                fill_buffer(buffer, 0, buffer_size, 0);
            }
        }

        if (write(fd, buffer, buffer_size) != buffer_size) {
            perror("generate_fake_sparse_file: write");
            return 1;
        }
    }

    fill_buffer(buffer, 0, buffer_size, 1);
    if (write(fd, buffer, remainder) != remainder) {
        perror("generate_fake_sparse_file: write remainder");
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: ./fake-sparse-file-generator <output_file> [block_size]\n");
        return 1;
    }

    char *output_file = argv[1];
    int buffer_size = DEF_BUF_LEN;

    if (argc == 3) {
        char *end;
        buffer_size = strtol(argv[2], &end, 10);
        if (argv[2] == end) {
            perror("main: parsing strtol");
            return 1;
        }
    }

    const int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("main: open output file");
        return 1;
    }

    char *buffer = calloc(buffer_size, sizeof (char));

    if (buffer == NULL) {
        perror("main: calloc");
        return 1;
    }

    const int generator_res = generate_fake_sparse_file(fd, buffer_size, buffer);

    free(buffer);

    if (close(fd) != 0) {
        perror("main: close output file");
        return 1;
    }

    return generator_res;
}
