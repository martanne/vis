enum { DA_INITIAL_CAP = 16 };

#define da_release(da) free((da)->data)

#define da_unordered_remove(da, index) do { \
	if ((index) < (da)->count - 1) \
		(da)->data[(index)] = (da)->data[(da)->count - 1]; \
	(da)->count--; \
} while(0)

#define da_ordered_remove(da, index) do { \
	if ((index) < (da)->count - 1) \
		memmove((da)->data + (index), (da)->data + (index) + 1, \
		        sizeof(*(da)->data) * ((da)->count - (index) - 1)); \
	(da)->count--; \
} while(0)

#define DA_COMPARE_FN(name) int name(const void *va, const void *vb)
#define da_sort(da, compare) qsort((da)->data, (da)->count, sizeof(*(da)->data), (compare))

#define da_reserve(vis, da, n) \
  (da)->data = da_reserve_(vis, (da)->data, &(da)->capacity, (da)->count + n, sizeof(*(da)->data))

#define da_push(vis, da) \
  ((da)->count == (da)->capacity \
    ? da_reserve(vis, da, 1), \
      (da)->data + (da)->count++ \
    : (da)->data + (da)->count++)


#ifndef VIS_CORE_H
typedef struct Vis Vis;
#define vis_oom(vis) abort()
#endif
static void *
da_reserve_(Vis *vis, void *data, VisDACount *capacity, VisDACount needed, size_t size)
{
	VisDACount cap = *capacity;

	if (!cap) cap = DA_INITIAL_CAP;
	while (cap < needed) cap *= 2;
	data = realloc(data, size * cap);
	if (unlikely(data == 0))
		vis_oom(vis);

	memset((char *)data + (*capacity * size), 0, size * (cap - *capacity));
	*capacity = cap;
	return data;
}

static void *
memory_scan_reverse(const void *memory, uint8_t byte, ptrdiff_t n)
{
	void *result = 0;
	if (n > 0) {
		const uint8_t *s = memory;
		while (n) if (s[--n] == byte) { result = (void *)(s + n); break; }
	}
	return result;
}

static void *
memory_scan_forward(const void *memory, uint8_t byte, ptrdiff_t n)
{
	const uint8_t *s = memory, *end = s + n;
	while (s != end && *s != byte) s++;
	void *result = (s != end) ? (void *)s : 0;
	return result;
}

static void
memory_copy(void *restrict dest, void *restrict src, u64 n)
{
	u8 *s = src, *d = dest;
	for (; n; n--) *d++ = *s++;
}

static str8
str8_from_c_str(const char *c_str)
{
	str8 result = {.data = (uint8_t *)c_str};
	if (c_str) while (*c_str) c_str++;
	result.length = (uint8_t *)c_str - result.data;
	return result;
}

static bool
str8_equal(str8 a, str8 b)
{
	bool result = a.length == b.length;
	for (ptrdiff_t i = 0; result && i < a.length; i++)
		result = a.data[i] == b.data[i];
	return result;
}

static bool
str8_case_ignore_equal(str8 a, str8 b)
{
	bool result = a.length == b.length;
	for (ptrdiff_t i = 0; result && i < a.length; i++)
		result = (a.data[i] & ~0x20u) == (b.data[i] & ~0x20u);
	return result;
}

static bool
str8_is_prefix(str8 prefix, str8 s)
{
	bool result = s.length >= prefix.length &&
	              str8_equal(prefix, (str8){.data = s.data, .length = prefix.length});
	return result;
}

static str8
str8_skip(str8 s, int64_t count)
{
	str8 result = s;
	if (count > 0) {
		result.data   += MIN(count, s.length);
		result.length -= MIN(count, s.length);
	}
	return result;
}

static str8
str8_skip_space(str8 s)
{
	str8 result = s;
	for (s64 i = 0; i < s.length && IsSpace(*result.data); result.data++, i++);
	result.length -= result.data - s.data;
	return result;
}

static str8
str8_trim_space(str8 s)
{
	str8 result = str8_skip_space(s);
	while (result.length > 0 && IsSpace(result.data[result.length - 1])) result.length--;
	return result;
}

static void
str8_split_at(str8 s, str8 *left, str8 *right, ptrdiff_t n)
{
	if (left)  *left  = s;
	if (right) *right = (str8){0};
	if (n >= 0 && n <= s.length) {
		if (left) *left = (str8){
			.length = n,
			.data   = s.data,
		};
		if (right) *right = (str8){
			.length = MAX(0, s.length - n - 1),
			.data   = s.data + n + 1,
		};
	}
}

static void
str8_split(str8 s, str8 *left, str8 *right, uint8_t byte)
{
	str8_split_at(s, left, right, (uint8_t *)memory_scan_forward(s.data, byte, s.length) - s.data);
}

static void
path_split(str8 path, str8 *directory, str8 *basename)
{
	str8 left;
	ptrdiff_t at = (uint8_t *)memory_scan_reverse(path.data, '/', path.length) - path.data;
	str8_split_at(path, &left, basename, at);

	if (basename && basename->length == 0 && at <= 0) {
		*basename = left;
		left      = (str8){0};
	}

	if (directory) *directory = left.length == 0 ? str8(".") : left;
}

// NOTE(rnp): returns 0 terminated str8
VIS_INTERNAL str8
vis_absolute_path(const char *name)
{
	str8 result = {0};
	if (!name)
		return result;

	str8 base, dir, string = str8_from_c_str((char *)name);
	path_split(string, &dir, &base);

	char path_resolved[PATH_MAX];
	char path_normalized[PATH_MAX];

	memcpy(path_normalized, dir.data, dir.length);
	path_normalized[dir.length] = 0;

	if (realpath(path_normalized, path_resolved) == path_resolved) {
		str8 resolved = str8_from_c_str(path_resolved);
		if (str8_equal(resolved, str8("/"))) {
			resolved.data[0] = 0;
			resolved.length  = 0;
		}

		int64_t total_length = base.length + resolved.length + 1;
		result.data = malloc(total_length + 1);
		if (result.data) {
			memcpy(result.data, resolved.data, resolved.length);
			memcpy(result.data + resolved.length + 1, base.data, base.length);
			result.data[resolved.length] = '/';
			result.data[total_length]    = 0;
			result.length = total_length;
		}
	}
	return result;
}

VIS_INTERNAL IntegerConversion
integer_conversion(str8 raw, IntegerConversionFlags flags)
{
	/* NOTE: place this on its own cacheline */
	static alignas(64) int8_t lut[64] = {
		 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};

	IntegerConversion result = {.unparsed = raw};

	int64_t i = 0, scale = 1;
	if (raw.length > 0 && raw.data[0] == '-') {
		scale = -1;
		i     =  1;
	}

	bool hex = (flags & IntegerConversionFlag_ForceHex) != 0;
	if ((flags & IntegerConversionFlag_NoAutoHex) == 0) {
		if (raw.length - i > 2 && raw.data[i] == '0' && (raw.data[1] == 'x' || raw.data[1] == 'X')) {
			hex  = 1;
			i   += 2;
		} else if (raw.length - i > 1 && raw.data[i] == '#') {
			hex  = 1;
			i   += 1;
		}
	}

	#define integer_conversion_body(radix, clamp) do {\
		for (; i < raw.length; i++) {\
			int64_t value = lut[MIN((uint8_t)(raw.data[i] - (uint8_t)'0'), clamp)];\
			if (value < 0) break;\
			if (result.as.U64 > (U64_MAX - (uint64_t)value) / radix) {\
				result.result = IntegerConversionResult_OutOfRange;\
				result.as.U64 = U64_MAX;\
				break;\
			}\
			result.as.U64 = radix * result.as.U64 + (uint64_t)value;\
		}\
	} while (0)

	if (hex) integer_conversion_body(16u, 63u);
	else     integer_conversion_body(10u, 15u);

	#undef integer_conversion_body

	if (result.result != IntegerConversionResult_OutOfRange) {
		result.unparsed = (str8){.length = raw.length - i, .data = raw.data + i};
		result.result   = IntegerConversionResult_Success;
		if (scale < 0) result.as.U64 = 0 - result.as.U64;
	}

	return result;
}
