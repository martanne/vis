#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

#define BUF_SIZE 1024

void buffer_init(Buffer *buf) {
	memset(buf, 0, sizeof *buf);
}

bool buffer_grow(Buffer *buf, size_t size) {
	if (size < BUF_SIZE)
		size = BUF_SIZE;
	if (buf->size < size) {
		if (buf->size > 0)
			size *= 2;
		char *data = realloc(buf->data, size);
		if (!data)
			return false;
		buf->size = size;
		buf->data = data;
	}
	return true;
}

void buffer_truncate(Buffer *buf) {
	buf->len = 0;
}

void buffer_release(Buffer *buf) {
	if (!buf)
		return;
	free(buf->data);
	buffer_init(buf);
}

bool buffer_put(Buffer *buf, const void *data, size_t len) {
	if (!buffer_grow(buf, len))
		return false;
	memcpy(buf->data, data, len);
	buf->len = len;
	return true;
}

bool buffer_append(Buffer *buf, const void *data, size_t len) {
	size_t rem = buf->size - buf->len;
	if (len > rem && !buffer_grow(buf, buf->size + len - rem))
		return false;
	memcpy(buf->data + buf->len, data, len);
	buf->len += len;
	return true;
}
