/*
 * @file readfile.c
 */


#define _POSIX_C_SOURCE 200809L   /* for POSIX functions/macros */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "readfile.h"


/* -------------------- FILE READER ------------------ */


char* read_file(const char* path, size_t* len_out)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "fstat(%s): %s\n", path, strerror(errno));
        close(fd);
        return NULL;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "%s: not a regular file\n", path);
        close(fd);
        return NULL;
    }

    size_t size = (size_t)st.st_size;
    char* buf = (char*)malloc(size);
    if (!buf) {
        fprintf(stderr, "malloc(%zu): %s\n", size, strerror(errno));
        close(fd);
        return NULL;
    }

    size_t total = 0;
    while (total < size) {
        ssize_t n = read(fd, buf + total, size - total);
        if (n == -1) {
            if (errno == EINTR) continue;
            fprintf(stderr, "read(%s): %s\n", path, strerror(errno));
            free(buf);
            close(fd);
            return NULL;
        }
        if (n == 0) break;
        total += (size_t)n;
    }

    if (len_out) *len_out = total;

    close(fd);
    return buf;
}
