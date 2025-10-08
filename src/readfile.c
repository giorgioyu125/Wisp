/*
 * @file readfile.c
 */


#define _POSIX_C_SOURCE 200809L /* for POSIX functions/macros */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mman.h>

#include "readfile.h"


/* -------------------- FILE READER ------------------ */

FileBuffer *read_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return NULL;

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return NULL;
    }

    size_t size = (size_t) st.st_size;
    char *file = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file == MAP_FAILED)
        return NULL;
    close(fd);

    FileBuffer *fb = malloc(sizeof(FileBuffer));
    if (!fb) {
        munmap(file, size);
        return NULL;
    }

    fb->data = file;
    fb->size = size;
    fb->is_mmaped = true;

    return fb;
}

void filebuffer_free(FileBuffer *fb)
{
    if (!fb)
        return;
    if (fb->is_mmaped) {
        munmap(fb->data, fb->size);
    } else {
        free(fb->data);
    }
    free(fb);
}
