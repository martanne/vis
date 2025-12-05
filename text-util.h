#ifndef TEXT_UTIL_H
#define TEXT_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include "text.h"

/* test whether the given range is valid (start <= end) */
#define text_range_valid(r) ((r)->start != EPOS && (r)->end != EPOS && (r)->start <= (r)->end)
/* get the size of the range (end-start) or zero if invalid */
#define text_range_size(r) (text_range_valid(r) ? (r)->end - (r)->start : 0)
/* create an empty / invalid range of size zero */
#define text_range_empty() (Filerange){.start = EPOS, .end = EPOS}
/* merge two ranges into a new one which contains both of them */
VIS_INTERNAL Filerange text_range_union(const Filerange*, const Filerange*);
/* get intersection of two ranges */
VIS_INTERNAL Filerange text_range_intersect(const Filerange*, const Filerange*);
/* create new range [min(a,b), max(a,b)] */
VIS_INTERNAL Filerange text_range_new(size_t a, size_t b);
/* test whether two ranges are equal */
VIS_INTERNAL bool text_range_equal(const Filerange*, const Filerange*);
/* test whether two ranges overlap */
VIS_INTERNAL bool text_range_overlap(const Filerange*, const Filerange*);
/* test whether a given position is within a certain range */
VIS_INTERNAL bool text_range_contains(const Filerange*, size_t pos);
/* count the number of graphemes in data */
VIS_INTERNAL int text_char_count(const char *data, size_t len);
/* get the approximate display width of data */
VIS_INTERNAL int text_string_width(const char *data, size_t len);

#endif
