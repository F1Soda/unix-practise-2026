#ifndef FILE_UTILS_H
#define FILE_UTILS_H
#include <stddef.h>
#include <stdio.h>

// Оберкти над операциями, которые при ошибке выводят сообщение

ssize_t read_block(int fd, void* buf, size_t n);

ssize_t write_block(int fd, const void* buf, size_t n);

int close_file(int fd);

#endif //FILE_UTILS_H