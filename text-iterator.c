#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* memrchr(3) is non-standard */
#endif
#include <limits.h>
#include <stddef.h>
#include <errno.h>
#include <wchar.h>
#include <string.h>
#include "text.h"
#include "util.h"

bool text_iterator_byte_get(const Iterator *it, char *b) {
	if (text_iterator_valid(it)) {
		const Text *txt = text_iterator_text(it);
		if (it->start <= it->text && it->text < it->end) {
			*b = *it->text;
			return true;
		} else if (it->pos == text_size(txt)) {
			*b = '\0';
			return true;
		}
	}
	return false;
}

bool text_iterator_byte_next(Iterator *it, char *b) {
	if (!text_iterator_has_next(it))
		return false;
	bool eof = true;
	if (it->text != it->end) {
		it->text++;
		it->pos++;
		eof = false;
	} else if (!text_iterator_has_prev(it)) {
		eof = false;
	}

	while (it->text == it->end) {
		if (!text_iterator_next(it)) {
			if (eof)
				return false;
			if (b)
				*b = '\0';
			return text_iterator_prev(it);
		}
	}

	if (b)
		*b = *it->text;
	return true;
}

bool text_iterator_byte_prev(Iterator *it, char *b) {
	if (!text_iterator_has_prev(it))
		return false;
	bool eof = !text_iterator_has_next(it);
	while (it->text == it->start) {
		if (!text_iterator_prev(it)) {
			if (!eof)
				return false;
			if (b)
				*b = '\0';
			return text_iterator_next(it);
		}
	}

	--it->text;
	--it->pos;

	if (b)
		*b = *it->text;
	return true;
}

bool text_iterator_byte_find_prev(Iterator *it, char b) {
	while (it->text) {
		const char *match = memrchr(it->start, b, it->text - it->start);
		if (match) {
			it->pos -= it->text - match;
			it->text = match;
			return true;
		}
		text_iterator_prev(it);
	}
	text_iterator_next(it);
	return false;
}

bool text_iterator_byte_find_next(Iterator *it, char b) {
	while (it->text) {
		const char *match = memchr(it->text, b, it->end - it->text);
		if (match) {
			it->pos += match - it->text;
			it->text = match;
			return true;
		}
		text_iterator_next(it);
	}
	text_iterator_prev(it);
	return false;
}

bool text_iterator_codepoint_next(Iterator *it, char *c) {
	while (text_iterator_byte_next(it, NULL)) {
		if (ISUTF8(*it->text)) {
			if (c)
				*c = *it->text;
			return true;
		}
	}
	return false;
}

bool text_iterator_codepoint_prev(Iterator *it, char *c) {
	while (text_iterator_byte_prev(it, NULL)) {
		if (ISUTF8(*it->text)) {
			if (c)
				*c = *it->text;
			return true;
		}
	}
	return false;
}

bool text_iterator_char_next(Iterator *it, char *c) {
	if (!text_iterator_codepoint_next(it, c))
		return false;
	mbstate_t ps = { 0 };
	const Text *txt = text_iterator_text(it);
	for (;;) {
		char buf[MB_LEN_MAX];
		size_t len = text_bytes_get(txt, it->pos, sizeof buf, buf);
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			return true;
		} else if (wclen == (size_t)-2) {
			return false;
		} else if (wclen == 0) {
			return true;
		} else {
			int width = wcwidth(wc);
			if (width != 0)
				return true;
			if (!text_iterator_codepoint_next(it, c))
				return false;
		}
	}
	return true;
}

bool text_iterator_char_prev(Iterator *it, char *c) {
	if (!text_iterator_codepoint_prev(it, c))
		return false;
	const Text *txt = text_iterator_text(it);
	for (;;) {
		char buf[MB_LEN_MAX];
		size_t len = text_bytes_get(txt, it->pos, sizeof buf, buf);
		wchar_t wc;
		mbstate_t ps = { 0 };
		size_t wclen = mbrtowc(&wc, buf, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			return true;
		} else if (wclen == (size_t)-2) {
			return false;
		} else if (wclen == 0) {
			return true;
		} else {
			int width = wcwidth(wc);
			if (width != 0)
				return true;
			if (!text_iterator_codepoint_prev(it, c))
				return false;
		}
	}
	return true;
}
