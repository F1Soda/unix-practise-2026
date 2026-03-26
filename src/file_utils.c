#include "file_utils.h"
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>


ssize_t read_block(const int fd, void* buf, const size_t n) {
    const ssize_t res = read(fd, buf, n);

    if (res != n && res != 0) {
        perror("read");
    }

    return res;
}

ssize_t write_block(const int fd, const void* buf, const size_t n) {
    const ssize_t res = write(fd, buf, n);

    if (res != n) {
        perror("write");
    }

    return res;
}

int close_file(const int fd) {
    if (close(fd) != 0) {
        perror("close");
        return 1;
    }

    return 0;
}