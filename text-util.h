#ifndef TEXT_UTIL_H
#define TEXT_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include "text.h"

/* test whether the given range is valid (start <= end) */
bool text_range_valid(const Filerange*);
/* get the size of the range (end-start) or zero if invalid */
size_t text_range_size(const Filerange*);
/* create an empty / invalid range of size zero */
Filerange text_range_empty(void);
/* merge two ranges into a new one which contains both of them */
Filerange text_range_union(const Filerange*, const Filerange*);
/* get intersection of two ranges */
Filerange text_range_intersect(const Filerange*, const Filerange*);
/* create new range [min(a,b), max(a,b)] */
Filerange text_range_new(size_t a, size_t b);
/* test whether two ranges are equal */
bool text_range_equal(const Filerange*, const Filerange*);
/* test whether two ranges overlap */
bool text_range_overlap(const Filerange*, const Filerange*);
/* test whether a given position is within a certain range */
bool text_range_contains(const Filerange*, size_t pos);
/* count the number of graphemes in data */
int text_char_count(const char *data, size_t len);
/* get the approximate display width of data */
int text_string_width(const char *data, size_t len);

#endif
