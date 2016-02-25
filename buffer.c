#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "util.h"

#define BUF_SIZE 1024

void buffer_init(Buffer *buf) {
	memset(buf, 0, sizeof *buf);
}

bool buffer_grow(Buffer *buf, size_t size) {
	/* ensure minimal buffer size, to avoid repeated realloc(3) calls */
	if (size < BUF_SIZE)
		size = BUF_SIZE;
	if (buf->size < size) {
		size = MAX(size, buf->size*2);
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

void buffer_clear(Buffer *buf) {
	buf->len = 0;
}

bool buffer_put(Buffer *buf, const void *data, size_t len) {
	if (!buffer_grow(buf, len))
		return false;
	memmove(buf->data, data, len);
	buf->len = len;
	return true;
}

bool buffer_put0(Buffer *buf, const char *data) {
	return buffer_put(buf, data, strlen(data)+1);
}

bool buffer_insert(Buffer *buf, size_t pos, const void *data, size_t len) {
	if (pos > buf->len)
		return false;
	if (!buffer_grow(buf, buf->len + len))
		return false;
	memmove(buf->data + pos + len, buf->data + pos, buf->len - pos);
	memcpy(buf->data + pos, data, len);
	buf->len += len;
	return true;
}

bool buffer_insert0(Buffer *buf, size_t pos, const char *data) {
	if (pos == 0)
		return buffer_prepend0(buf, data);
	if (pos == buf->len)
		return buffer_append0(buf, data);
	return buffer_insert(buf, pos, data, strlen(data));
}

bool buffer_append(Buffer *buf, const void *data, size_t len) {
	return buffer_insert(buf, buf->len, data, len);
}

bool buffer_append0(Buffer *buf, const char *data) {
	if (buf->len > 0 && buf->data[buf->len-1] == '\0')
		buf->len--;
	return buffer_append(buf, data, strlen(data)) && buffer_append(buf, "\0", 1);
}

bool buffer_prepend(Buffer *buf, const void *data, size_t len) {
	return buffer_insert(buf, 0, data, len);
}

bool buffer_prepend0(Buffer *buf, const char *data) {
	return buffer_prepend(buf, data, strlen(data) + (buf->len == 0));
}

size_t buffer_length0(Buffer *buf) {
	size_t len = buf->len;
	if (len > 0 && buf->data[len-1] == '\0')
		len--;
	return len;
}
