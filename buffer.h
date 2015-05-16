#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"

typedef struct {       /* a dynamically growing buffer storing arbitrary data */
	char *data;    /* NULL if empty */
	size_t len;    /* current length of data */
	size_t size;   /* maximal capacity of the buffer */
} Buffer;

/* initalize a (stack allocated) Buffer insteance */
void buffer_init(Buffer*);
/* relase/free all data stored in this buffer, reset size to zero */
void buffer_release(Buffer*);
/* reserve space to store at least size bytes in this buffer.*/
bool buffer_grow(Buffer*, size_t size);
/* truncate buffer, but keep associated memory region for further data */
void buffer_truncate(Buffer*);
/* replace buffer content with given data, growing the buffer if needed */
bool buffer_put(Buffer*, const void *data, size_t len);
/* append futher content to the end of the buffer data */
bool buffer_append(Buffer*, const void *data, size_t len);

#endif
