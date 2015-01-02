#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include "text.h"

typedef struct {
	char *data;    /* NULL if empty */
	size_t len;    /* current length of data */
	size_t size;   /* maximal capacity of the buffer */
} Buffer;

void buffer_free(Buffer *buf);
bool buffer_alloc(Buffer *buf, size_t size);
void buffer_truncate(Buffer *buf);
bool buffer_put(Buffer *buf, const void *data, size_t len);
bool buffer_append(Buffer *buf, const void *data, size_t len);

#endif
