static uint32_t
utf8_encode(uint8_t out[4], uint32_t cp)
{
	uint32_t result;
	if (cp <= 0x7F) {
		out[0] = cp & 0x7F;
		result = 1;
	} else if (cp <= 0x7FF) {
		result = 2;
		out[0] = ((cp >>  6) & 0x1F) | 0xC0;
		out[1] = ((cp >>  0) & 0x3F) | 0x80;
	} else if (cp <= 0xFFFF) {
		result = 3;
		out[0] = ((cp >> 12) & 0x0F) | 0xE0;
		out[1] = ((cp >>  6) & 0x3F) | 0x80;
		out[2] = ((cp >>  0) & 0x3F) | 0x80;
	} else if (cp <= 0x10FFFF) {
		result = 4;
		out[0] = ((cp >> 18) & 0x07) | 0xF0;
		out[1] = ((cp >> 12) & 0x3F) | 0x80;
		out[2] = ((cp >>  6) & 0x3F) | 0x80;
		out[3] = ((cp >>  0) & 0x3F) | 0x80;
	} else {
		//out[0] = '?';
		result = 0;
	}
	return result;
}

#define zero_struct(s) memset((s), 0, sizeof(*s))

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

static void *
da_reserve_(Vis *vis, void *data, VisDACount *capacity, VisDACount needed, size_t size)
{
	VisDACount cap = *capacity;

	if (!cap) cap = DA_INITIAL_CAP;
	while (cap < needed) cap *= 2;
	data = realloc(data, size * cap);
	if (unlikely(data == 0))
		longjmp(vis->oom_jmp_buf, 1);

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

static str8
str8_from_c_str(char *c_str)
{
	str8 result = {
		.length = strlen(c_str),
		.data   = (uint8_t *)c_str,
	};
	return result;
}

static void
str8_split_at(str8 s, str8 *left, str8 *right, ptrdiff_t n)
{
	if (left)  *left  = (str8){0};
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
path_split(str8 path, str8 *directory, str8 *basename)
{
	str8_split_at(path, directory, basename,
	              (uint8_t *)memory_scan_reverse(path.data, '/', path.length) - path.data);
	if (directory && directory->length == 0) *directory = str8(".");
	if (basename  && basename->length  == 0) *basename  = path;
}

static char *
absolute_path(const char *name)
{
	if (!name)
		return 0;

	str8 base, dir, string = str8_from_c_str((char *)name);
	path_split(string, &dir, &base);

	char path_resolved[PATH_MAX];
	char path_normalized[PATH_MAX];

	memcpy(path_normalized, dir.data, dir.length);
	path_normalized[dir.length] = 0;

	char *result = 0;
	if (realpath(path_normalized, path_resolved) == path_resolved) {
		if (strcmp(path_resolved, "/") == 0)
			path_resolved[0] = 0;

		snprintf(path_normalized, sizeof(path_normalized), "%s/%.*s",
		         path_resolved, (int)base.length, base.data);
		result = strdup(path_normalized);
	}
	return result;
}
