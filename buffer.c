#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "buffer.h"
#include "util.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

void buffer_init(Buffer *buf) {
	memset(buf, 0, sizeof *buf);
}

bool buffer_reserve(Buffer *buf, size_t size) {
	/* ensure minimal buffer size, to avoid repeated realloc(3) calls */
	if (size < BUFFER_SIZE)
		size = BUFFER_SIZE;
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

bool buffer_grow(Buffer *buf, size_t len) {
	size_t size;
	if (!addu(buf->len, len, &size))
		return false;
	return buffer_reserve(buf, size);
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
	if (!buffer_reserve(buf, len))
		return false;
	memmove(buf->data, data, len);
	buf->len = len;
	return true;
}

bool buffer_put0(Buffer *buf, const char *data) {
	return buffer_put(buf, data, strlen(data)+1);
}

bool buffer_remove(Buffer *buf, size_t pos, size_t len) {
	size_t end;
	if (len == 0)
		return true;
	if (!addu(pos, len, &end) || end > buf->len)
		return false;
	memmove(buf->data + pos, buf->data + pos + len, buf->len - pos - len);
	buf->len -= len;
	return true;
}

bool buffer_insert(Buffer *buf, size_t pos, const void *data, size_t len) {
	if (pos > buf->len)
		return false;
	if (len == 0)
		return true;
	if (!buffer_grow(buf, len))
		return false;
	size_t move = buf->len - pos;
	if (move > 0)
		memmove(buf->data + pos + len, buf->data + pos, move);
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

static bool buffer_vappendf(Buffer *buf, const char *fmt, va_list ap) {
	va_list ap_save;
	va_copy(ap_save, ap);
	int len = vsnprintf(NULL, 0, fmt, ap);
	if (len == -1 || !buffer_grow(buf, len+1)) {
		va_end(ap_save);
		return false;
	}
	size_t nul = (buf->len > 0 && buf->data[buf->len-1] == '\0') ? 1 : 0;
	buf->len -= nul;
	bool ret = vsnprintf(buf->data+buf->len, len+1, fmt, ap_save) == len;
	buf->len += ret ? (size_t)len+1 : nul;
	va_end(ap_save);
	return ret;
}

bool buffer_appendf(Buffer *buf, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	bool ret = buffer_vappendf(buf, fmt, ap);
	va_end(ap);
	return ret;
}

bool buffer_printf(Buffer *buf, const char *fmt, ...) {
	buffer_clear(buf);
	va_list ap;
	va_start(ap, fmt);
	bool ret = buffer_vappendf(buf, fmt, ap);
	va_end(ap);
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

size_t buffer_capacity(Buffer *buf) {
	return buf->size;
}

const char *buffer_content(Buffer *buf) {
	return buf->data;
}

const char *buffer_content0(Buffer *buf) {
	if (buf->len == 0 || !buffer_terminate(buf))
		return "";
	return buf->data;
}

char *buffer_move(Buffer *buf) {
	char *data = buf->data;
	buffer_init(buf);
	return data;
}
