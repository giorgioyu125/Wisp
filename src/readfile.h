/**
 * @file readfile.h
 */
#ifndef READFILE_H
#define READFILE_H

#include <stddef.h>

/* ------------------------ FILE BUF ----------------- */

typedef struct {
    char *data;
    size_t size;
    bool is_mmaped;
} FileBuffer;


/* -------------------- FILE READER ------------------ */

/**
 * @brief  Read the entire contents of a file into a dynamically
 *         allocated buffer.
 *
 * @param[in]  path     Path to the file that will be read
 *                      (must reference a regular, readable file).
 * @param[out] len_out  Optional pointer that, on success, receives the
 *                      number of bytes actually read. 
 *                      Pass `NULL` if the length is not needed.
 *
 * @return Pointer to the newly allocated buffer on success;  
 *         `NULL` on failure.
 *
 * @warning Ownership of the memory is transferred to the caller, who must
 *			eventually free it with `filebuffer_free()`.
 */
FileBuffer *read_file(const char *path);

/*
 * @brief This function frees the memory of the file read through the
 *        read_file function by leveraging the FileBuffer struct.
 */
void filebuffer_free(FileBuffer * fb);


#endif
