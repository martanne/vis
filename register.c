#include <stdlib.h>
#include <string.h>

#include "register.h"
#include "util.h"

#define REG_SIZE 1024

bool register_alloc(Register *reg, size_t size) {
	if (size < REG_SIZE)
		size = REG_SIZE;
	if (reg->size < size) {
		reg->data = realloc(reg->data, size);
		if (!reg->data) {
			reg->size = 0;
			reg->len = 0;
			return false;
		}
		reg->size = size;
	}
	return true;
}

void register_free(Register *reg) {
	if (!reg)
		return;
	free(reg->data);
	reg->data = NULL;
	reg->len = 0;
	reg->size = 0;
}

bool register_store(Register *reg, const char *data, size_t len) {
	if (!register_alloc(reg, len))
		return false;
	memcpy(reg->data, data, len);
	reg->len = len;
	return true;
}

bool register_put(Register *reg, Text *txt, Filerange *range) {
	size_t len = range->end - range->start;
	if (!register_alloc(reg, len))
		return false;
	reg->len = text_bytes_get(txt, range->start, len, reg->data);
	return true;
}

bool register_append(Register *reg, Text *txt, Filerange *range) {
	size_t rem = reg->size - reg->len;
	size_t len = range->end - range->start;
	if (len > rem && !register_alloc(reg, reg->size + len - rem))
		return false;
	reg->len += text_bytes_get(txt, range->start, len, reg->data + reg->len);
	return true;
}
