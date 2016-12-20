#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

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

bool buffer_terminate(Buffer *buf) {
	return !buf->data || buf->len == 0 || buf->data[buf->len-1] == '\0' ||
	        buffer_append(buf, "\0", 1);
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

bool buffer_remove(Buffer *buf, size_t pos, size_t len) {
	if (len == 0)
		return true;
	if (pos + len > buf->len)
		return false;
	memmove(buf->data + pos, buf->data + pos + len, buf->len - pos - len);
	buf->len -= len;
	return true;
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
	size_t nul = (buf->len > 0 && buf->data[buf->len-1] == '\0') ? 1 : 0;
	buf->len -= nul;
	bool ret = buffer_append(buf, data, strlen(data)+1);
	if (!ret)
		buf->len += nul;
	return ret;
}

bool buffer_prepend(Buffer *buf, const void *data, size_t len) {
	return buffer_insert(buf, 0, data, len);
}

bool buffer_prepend0(Buffer *buf, const char *data) {
	return buffer_prepend(buf, data, strlen(data) + (buf->len == 0));
}

bool buffer_printf(Buffer *buf, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	bool ret = buffer_vprintf(buf, fmt, ap);
	va_end(ap);
	return ret;
}

bool buffer_vprintf(Buffer *buf, const char *fmt, va_list ap) {
	va_list ap_save;
	va_copy(ap_save, ap);
	int len = vsnprintf(NULL, 0, fmt, ap);
	if (len == -1 || !buffer_grow(buf, len+1)) {
		va_end(ap_save);
		return false;
	}
	bool ret = vsnprintf(buf->data, len+1, fmt, ap_save) == len;
	if (ret)
		buf->len = len+1;
	va_end(ap_save);
	return ret;
}

size_t buffer_length0(Buffer *buf) {
	size_t len = buf->len;
	if (len > 0 && buf->data[len-1] == '\0')
		len--;
	return len;
}

size_t buffer_length(Buffer *buf) {
	return buf->len;
}

const char *buffer_content(Buffer *buf) {
	return buf->data;
}

const char *buffer_content0(Buffer *buf) {
	if (buf->len == 0 || (buf->data[buf->len-1] != '\0' && !buffer_append(buf, "\0", 1)))
		return "";
	return buf->data;
}
