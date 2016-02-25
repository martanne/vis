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
bool buffer_grow(Buffer*, size_t size);
/* truncate buffer, but keep associated memory region for further data */
void buffer_truncate(Buffer*);
/* replace buffer content with given data, growing the buffer if needed */
bool buffer_put(Buffer*, const void *data, size_t len);
/* same but with NUL-terminated data */
bool buffer_put0(Buffer*, const char *data);
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
/* return length of a buffer without trailing NUL byte */
size_t buffer_length0(Buffer*);

#endif
