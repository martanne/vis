#include <stdlib.h>
#include <string.h>

#include "register.h"
#include "buffer.h"
#include "text.h"
#include "util.h"

void register_release(Register *reg) {
	buffer_release(&reg->buf);
}

const char *register_get(Register *reg, size_t *len) {
	switch (reg->type) {
	case REGISTER_NORMAL:
		*len = reg->buf.len;
		return reg->buf.data;
	case REGISTER_BLACKHOLE:
	default:
		*len = 0;
		return NULL;
	}
}

bool register_put(Register *reg, Text *txt, Filerange *range) {
	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		size_t len = text_range_size(range);
		if (!buffer_grow(&reg->buf, len))
			return false;
		reg->buf.len = text_bytes_get(txt, range->start, len, reg->buf.data);
		return true;
	}
	case REGISTER_BLACKHOLE:
		return true;
	default:
		return false;
	}
}

bool register_append(Register *reg, Text *txt, Filerange *range) {
	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		size_t len = text_range_size(range);
		if (!buffer_grow(&reg->buf, reg->buf.len + len))
			return false;
		reg->buf.len += text_bytes_get(txt, range->start, len, reg->buf.data + reg->buf.len);
		return true;
	}
	default:
		return false;
	}
}
