/**
 * @file readfile.h
 */
#ifndef READFILE_H
#define READFILE_H

#include <stddef.h>

/* -------------------- FILE READER ------------------ */

/**
 * @brief  Read the entire contents of a file into a dynamically
 *         allocated, NUL-terminated buffer.
 *
 * The function opens the file designated by @p path, determines its size,
 * allocates a buffer of <em>size&nbsp;+&nbsp;1</em> bytes, reads the
 * contents, appends a terminating <tt>'\0'</tt>, and returns a pointer to
 * that buffer.  
 *
 * Ownership of the memory is transferred to the caller, who must
 * eventually free it with `free()`.
 *
 * @param[in]  path     Path to the file that will be read
 *                      (must reference a regular, readable file).
 * @param[out] len_out  Optional pointer that, on success, receives the
 *                      number of bytes actually read (excluding the final
 *                      NUL).  Pass `NULL` if the length is not needed.
 *
 * @return Pointer to the newly allocated buffer on success;  
 *         `NULL` on failure, in which case `errno` is set to indicate the
 *         error (e.g., `ENOENT`, `EACCES`, `ENOMEM`, …).
 *
 * @note The returned buffer is NUL-terminated so it can be treated as a
 *       C string *if* the file does not contain embedded NUL bytes.  When
 *       handling arbitrary binary data, rely on the length reported
 *       through @p len_out instead of `strlen()`.
 *
 * @warning Mixing high-level stdio (`FILE *`, `fread()`, …) with the
 *          low-level descriptor used internally by this function on the
 *          same file may lead to subtle bugs; avoid doing so unless you
 *          thoroughly flush and synchronize the respective buffers.
 *
 * @see open(2), read(2), fstat(2), free(3), errno(3)
 */
char* read_file(const char* path, size_t* len_out);


#endif
