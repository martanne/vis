#include <stdlib.h>
#include <string.h>

#include "register.h"
#include "buffer.h"
#include "text.h"
#include "util.h"

void register_free(Register *reg) {
	buffer_free((Buffer*)reg);
}

bool register_put(Register *reg, Text *txt, Filerange *range) {
	size_t len = range->end - range->start;
	if (!buffer_alloc((Buffer*)reg, len))
		return false;
	reg->len = text_bytes_get(txt, range->start, len, reg->data);
	return true;
}

bool register_append(Register *reg, Text *txt, Filerange *range) {
	size_t rem = reg->size - reg->len;
	size_t len = range->end - range->start;
	if (len > rem && !buffer_alloc((Buffer*)reg, reg->size + len - rem))
		return false;
	reg->len += text_bytes_get(txt, range->start, len, reg->data + reg->len);
	return true;
}
