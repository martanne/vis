#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "text.h"

static bool text_vprintf(Text *txt, size_t pos, const char *format, va_list ap) {
	va_list ap_save;
	va_copy(ap_save, ap);
	int len = vsnprintf(NULL, 0, format, ap);
	if (len == -1) {
		va_end(ap_save);
		return false;
	}
	char *buf = malloc(len+1);
	bool ret = buf && (vsnprintf(buf, len+1, format, ap_save) == len) && text_insert(txt, pos, buf, len);
	free(buf);
	va_end(ap_save);
	return ret;
}

bool text_appendf(Text *txt, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	bool ret = text_vprintf(txt, text_size(txt), format, ap);
	va_end(ap);
	return ret;
}

bool text_printf(Text *txt, size_t pos, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	bool ret = text_vprintf(txt, pos, format, ap);
	va_end(ap);
	return ret;
}

bool text_byte_get(const Text *txt, size_t pos, char *byte) {
	return text_bytes_get(txt, pos, 1, byte);
}

size_t text_bytes_get(const Text *txt, size_t pos, size_t len, char *buf) {
	if (!buf)
		return 0;
	char *cur = buf;
	size_t rem = len;
	for (Iterator it = text_iterator_get(txt, pos);
	     text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		if (rem == 0)
			break;
		size_t piece_len = it.end - it.text;
		if (piece_len > rem)
			piece_len = rem;
		if (piece_len) {
			memcpy(cur, it.text, piece_len);
			cur += piece_len;
			rem -= piece_len;
		}
	}
	return len - rem;
}

char *text_bytes_alloc0(const Text *txt, size_t pos, size_t len) {
	if (len == SIZE_MAX)
		return NULL;
	char *buf = malloc(len+1);
	if (!buf)
		return NULL;
	len = text_bytes_get(txt, pos, len, buf);
	buf[len] = '\0';
	return buf;
}
