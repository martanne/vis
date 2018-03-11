#include "text-util.h"
#include "util.h"
#include <wchar.h>
#include <errno.h>
#include <stdlib.h>

bool text_range_valid(const Filerange *r) {
	return r->start != EPOS && r->end != EPOS && r->start <= r->end;
}

size_t text_range_size(const Filerange *r) {
	return text_range_valid(r) ? r->end - r->start : 0;
}

Filerange text_range_empty(void) {
	return (Filerange){ .start = EPOS, .end = EPOS };
}

Filerange text_range_union(const Filerange *r1, const Filerange *r2) {
	if (!text_range_valid(r1))
		return *r2;
	if (!text_range_valid(r2))
		return *r1;
	return (Filerange) {
		.start = MIN(r1->start, r2->start),
		.end = MAX(r1->end, r2->end),
	};
}

Filerange text_range_intersect(const Filerange *r1, const Filerange *r2) {
	if (!text_range_overlap(r1, r2))
		return text_range_empty();
	return text_range_new(MAX(r1->start, r2->start), MIN(r1->end, r2->end));
}

Filerange text_range_new(size_t a, size_t b) {
	return (Filerange) {
		.start = MIN(a, b),
		.end = MAX(a, b),
	};
}

bool text_range_equal(const Filerange *r1, const Filerange *r2) {
	if (!text_range_valid(r1) && !text_range_valid(r2))
		return true;
	return r1->start == r2->start && r1->end == r2->end;
}

bool text_range_overlap(const Filerange *r1, const Filerange *r2) {
	if (!text_range_valid(r1) || !text_range_valid(r2))
		return false;
	return r1->start < r2->end && r2->start < r1->end;
}

bool text_range_contains(const Filerange *r, size_t pos) {
	return text_range_valid(r) && r->start <= pos && pos <= r->end;
}

int text_char_count(const char *data, size_t len) {
	int count = 0;
	mbstate_t ps = { 0 };
	while (len > 0) {
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, data, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			ps = (mbstate_t){0};
			count++;
			while (!ISUTF8(*data))
				data++, len--;
		} else if (wclen == (size_t)-2) {
			break;
                } else if (wclen == 0) {
			count++;
			data++;
			len--;
		} else {
			int width = wcwidth(wc);
			if (width != 0)
				count++;
			data += wclen;
			len -= wclen;
                }
	}
	return count;
}

int text_string_width(const char *data, size_t len) {

	int width = 0;
	mbstate_t ps = { 0 };
	const char *s = data;

	while (len > 0) {
		wchar_t wc;
		size_t wclen = mbrtowc(&wc, s, len, &ps);
		if (wclen == (size_t)-1 && errno == EILSEQ) {
			ps = (mbstate_t){0};
			/* assume a replacement symbol will be displayed */
			width++;
			wclen = 1;
		} else if (wclen == (size_t)-2) {
			/* do nothing, advance to next character */
			wclen = 1;
		} else if (wclen == 0) {
			/* assume NUL byte will be displayed as ^@ */
			width += 2;
			wclen = 1;
		} else if (wc == L'\t') {
			width++;
			wclen = 1;
		} else {
			int w = wcwidth(wc);
			if (w == -1)
				w = 2; /* assume non-printable will be displayed as ^{char} */
			width += w;
		}
		len -= wclen;
		s += wclen;
	}

	return width;
}
