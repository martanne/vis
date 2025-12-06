#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdbool.h>

#define LENGTH(x)  ((int)(sizeof (x) / sizeof *(x)))
#define MIN(a, b)  ((a) > (b) ? (b) : (a))
#define MAX(a, b)  ((a) < (b) ? (b) : (a))

/* is c the start of a utf8 sequence? */
#define ISUTF8(c)   (((c)&0xC0)!=0x80)
#define ISASCII(ch) ((unsigned char)ch < 0x80)

#if GCC_VERSION>=5004000 || CLANG_VERSION>=4000000
#define addu __builtin_add_overflow
#else
static inline bool addu(size_t a, size_t b, size_t *c) {
	if (SIZE_MAX - a < b)
		return false;
	*c = a + b;
	return true;
}
#endif

/* Needed for building on GNU Hurd */

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#endif /* UTIL_H */
