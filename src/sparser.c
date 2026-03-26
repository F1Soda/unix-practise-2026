#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "constants.h"
#include "file_utils.h"

int sparse(const int input_fd, const int output_fd, const int buffer_size, char *buffer) {
    ssize_t bytes_read;

    off_t skip = 0;
    while ((bytes_read = read_block(input_fd, buffer, buffer_size)) > 0) {
        int is_empty = 1;

        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] != 0) {
                is_empty = 0;
                break;
            }
        }

        if (is_empty) {
            // Вместо того чтобы постоянно вызывать lseek будем это делать только
            // после того как встретим блок с данными. А пока просто будем агрегировать офсет
            skip += bytes_read;
        } else {
            if (skip > 0) {
                if (lseek(output_fd, skip, SEEK_CUR) == -1) {
                    perror("lseek");
                    return 1;
                }
                skip = 0;
            }

            if (write_block(output_fd, buffer, bytes_read) != bytes_read) {
                return 1;
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: ./sparser <output_file> [input_file] [block_size]\n");
    }

    const char *output_file = argv[1];
    const int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1) {
        perror("open");
        return 1;
    }

    int input_fd = 0;
    if (argc >= 3) {
        char *input_file = argv[2];
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            perror("open");
            return 1;
        }
    }

    int buffer_size = DEF_BUF_LEN;
    if (argc == 4) {
        char *end;
        buffer_size = strtol(argv[3], &end, 10);
        if (argv[3] == end) {
            perror("Failed to parse buffer size");
            return 1;
        }
    }

    char *buffer = calloc(buffer_size, sizeof(char));

    sparse(input_fd, output_fd, buffer_size, buffer);

    free(buffer);
    return (close_file(output_fd) == 0 && close_file(input_fd) == 0) - 1;
}
