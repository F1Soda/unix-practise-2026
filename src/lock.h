#ifndef LOCK_H
#define LOCK_H

typedef struct {
    int fd;
    char *name;
} Lock;

Lock *get_lock(const char *filename);

int unlock(Lock *lock);

#endif //LOCK_H
