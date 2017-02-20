#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"

/* a dynamically growing buffer storing arbitrary data, used for registers/macros */
typedef struct {
	char *data;    /* NULL if empty */
	size_t len;    /* current length of data */
	size_t size;   /* maximal capacity of the buffer */
} Buffer;

/* initalize a (stack allocated) Buffer instance */
void buffer_init(Buffer*);
/* release/free all data stored in this buffer, reset size to zero */
void buffer_release(Buffer*);
/* set buffer size to zero, keep allocated memory */
void buffer_clear(Buffer*);
/* reserve space to store at least size bytes in this buffer.*/
bool buffer_reserve(Buffer*, size_t size);
/* if buffer is not empty, make sure it is NUL terminated */
bool buffer_terminate(Buffer*);
/* replace buffer content with given data, growing the buffer if needed */
bool buffer_put(Buffer*, const void *data, size_t len);
/* same but with NUL-terminated data */
bool buffer_put0(Buffer*, const char *data);
/* remove len bytes from the buffer starting at pos */
bool buffer_remove(Buffer*, size_t pos, size_t len);
/* insert arbitrary data of length len at pos (in [0, buf->len]) */
bool buffer_insert(Buffer*, size_t pos, const void *data, size_t len);
/* insert NUL-terminate data at pos (in [0, buf->len]) */
bool buffer_insert0(Buffer*, size_t pos, const char *data);
/* append futher content to the end of the buffer data */
bool buffer_append(Buffer*, const void *data, size_t len);
/* append NUl-terminated data */
bool buffer_append0(Buffer*, const char *data);
/* insert new data at the start of the buffer */
bool buffer_prepend(Buffer*, const void *data, size_t len);
/* prepend NUL-terminated data */
bool buffer_prepend0(Buffer*, const char *data);
/* set formatted buffer content, ensures NUL termination on success */
bool buffer_printf(Buffer*, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/* append formatted buffer content, ensures NUL termination on success */
bool buffer_appendf(Buffer*, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/* return length of a buffer without trailing NUL byte */
size_t buffer_length0(Buffer*);
/* return length of a buffer including possible NUL byte */
size_t buffer_length(Buffer*);
/* return current maximal capacity in bytes of this buffer */
size_t buffer_capacity(Buffer*);
/* pointer to buffer data, guaranteed to return a NUL terminated
 * string even if buffer is empty */
const char *buffer_content0(Buffer*);
/* pointer to buffer data, might be NULL if empty, might not be NUL terminated */
const char *buffer_content(Buffer*);
/* steal underlying buffer data, caller is responsible to free(3) it,
 * similar to move semantics in some programming languages */
char *buffer_move(Buffer*);

#endif
