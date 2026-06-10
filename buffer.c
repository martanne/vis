#include "buffer.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

VIS_INTERNAL bool
buffer_reserve(Buffer *b, s64 size)
{
	bool result = true;
	size = MAX(size, BUFFER_SIZE);
	if (b->size < size) {
		size = MAX(size, b->size * 2);
		char *data = realloc(b->data, size);
		result = data != 0;
		if (result) {
			b->size = size;
			b->data = data;
		}
	}
	return result;
}

VIS_INTERNAL bool
buffer_grow(Buffer *b, s64 length)
{
	bool result = buffer_reserve(b, b->length + length);
	return result;
}

VIS_INTERNAL void
vis_buffer_terminate(Buffer *b)
{
	if (b->data) {
		if (b->length == b->size) buffer_grow(b, 1);
		b->data[MIN(b->length, b->size - 1)] = 0;
		if (b->length == b->size) b->length--;
	}
}

void buffer_release(Buffer *buf) {
	if (!buf)
		return;
	free(buf->data);
	*buf = (Buffer){0};
}

VIS_INTERNAL bool
buffer_put(Buffer *b, const void *data, s64 length)
{
	// TODO(rnp): register code wants to pass in overlapping data
	//assert(!Between((char *)data, b->data, b->data + b->size));

	bool result = buffer_reserve(b, length);
	if (result) {
		memmove(b->data, data, length);
		b->length = length;
	}
	return result;
}

VIS_INTERNAL bool
vis_buffer_put_str8(Buffer *b, str8 s)
{
	bool result = buffer_put(b, s.data, s.length);
	return result;
}

VIS_INTERNAL bool
buffer_remove(Buffer *b, s64 at, s64 length)
{
	bool result = Between(at, 0, b->length) && Between(length, 0, b->length - at);
	if (result) {
		memmove(b->data + at, b->data + at + length, b->length - at - length);
		b->length -= length;
	}
	return result;
}

VIS_INTERNAL bool
vis_buffer_insert(Buffer *b, s64 at, const void *data, s64 length)
{
	bool result = at <= b->length;
	if (result && length > 0) {
		result = buffer_grow(b, length);
		if (result) {
			s64 move = b->length - at;
			if (move > 0) memmove(b->data + at + length, b->data + at, move);
			memcpy(b->data + at, data, length);
			b->length += length;
		}
	}
	return result;
}

VIS_INTERNAL bool
buffer_append(Buffer *b, const void *data, s64 length)
{
	bool result = vis_buffer_insert(b, b->length, data, length);
	return result;
}

VIS_INTERNAL bool
vis_buffer_append0(Buffer *buf, const char *data)
{
	bool result = buffer_append(buf, data, str8_from_c_str(data).length);
	return result;
}

VIS_INTERNAL bool
vis_buffer_vappendf(Buffer *b, const char *fmt, va_list ap)
{
	va_list ap_copy;
	va_copy(ap_copy, ap);

	s64  remaining = b->size - b->length;
	s64  length = vsnprintf(b->data + b->length, remaining, fmt, ap);
	// NOTE(rnp): vsnprintf will replace the last byte with 0 if there is exactly enough
	// space for the data if the 0 wasn't included. therefore we must always ensure at
	// least 1 extra byte gets added.
	bool result = length < remaining;
	if (!result && buffer_reserve(b, b->size + MAX(1, length - remaining)))
		length = vsnprintf(b->data + b->length, b->size - b->length, fmt, ap_copy);

	result = length <= b->size - b->length;
	if (result) b->length += length;

	va_end(ap_copy);

	return result;
}

VIS_INTERNAL bool
vis_buffer_appendf(Buffer *buf, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	bool result = vis_buffer_vappendf(buf, fmt, ap);
	va_end(ap);
	return result;
}

VIS_INTERNAL const char *
buffer_content0(Buffer *buf)
{
	vis_buffer_terminate(buf);
	const char *result = buf->length == 0 ? "" : buf->data;
	return result;
}

ssize_t read_into_buffer(void *context, char *data, size_t len) {
	buffer_append(context, data, len);
	return len;
}
