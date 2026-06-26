#ifndef UTIL_H
#define UTIL_H

#ifndef VERSION
  #define VERSION "unknown"
#endif
#ifndef VIS_API
  #define VIS_API 0
#endif

#ifndef VIS_INTERNAL
  #define VIS_INTERNAL static
#endif
#ifndef CONFIG_HELP
  #define CONFIG_HELP 1
#endif
#ifndef CONFIG_CURSES
  #define CONFIG_CURSES 0
#endif
#ifndef CONFIG_LUA
  #define CONFIG_LUA 0
#endif
#ifndef CONFIG_LPEG
  #define CONFIG_LPEG 0
#endif
#ifndef CONFIG_TRE
  #define CONFIG_TRE 0
#endif
#ifndef CONFIG_SELINUX
  #define CONFIG_SELINUX 0
#endif
#ifndef CONFIG_ACL
  #define CONFIG_ACL 0
#endif

#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#if CONFIG_ACL
#include <sys/acl.h>
#endif
#if CONFIG_SELINUX
#include <selinux/selinux.h>
#endif

#define InvalidCodePath assert(0)

#if defined(__clang__) || defined(__GNUC__)
#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

#define alignas(n)   __attribute__((aligned(n)))

#else
#define likely(x)    (x)
#define unlikely(x)  (x)

#define alignas(n)
#endif

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*(a)))
#endif

#define U64_MAX    (0xFFFFFFFFFFFFFFFFull)
#define S64_MAX    (0x7FFFFFFFFFFFFFFFll)
#define S32_MAX    (0x7FFFFFFFl)

#define LENGTH(x)  ((int)(sizeof (x) / sizeof *(x)))
#define MIN(a, b)  ((a) > (b) ? (b) : (a))
#define MAX(a, b)  ((a) < (b) ? (b) : (a))

#define Between(x, a, b) ((x) >= (a) && (x) <= (b))
#define Clamp(x, a, b)   (((x) < (a)) ? (a) : ((x) > (b)) ? (b) : (x))

/* is c the start of a utf8 sequence? */
#define ISUTF8(c)     (((c)&0xC0)!=0x80)
#define ISASCII(ch)   ((unsigned char)ch < 0x80)

#define IsBlank(c)    ((c) == ' ' || (c) == '\t')
#define IsSpace(c)    ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define IsBoundary(c) (isboundary((unsigned char)c))

#define zero_struct(s) memset((s), 0, sizeof(*(s)))

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


typedef size_t    uz;
typedef int64_t   s64;
typedef uint64_t  u64;
typedef int32_t   s32;
typedef uint32_t  u32;
typedef int16_t   s16;
typedef uint16_t  u16;
typedef int8_t    s8;
typedef uint8_t   u8;
typedef char      c8;

#define VisDACount s32

typedef struct {
	ptrdiff_t  length;
	uint8_t   *data;
} str8;
#define str8(s)      (str8){.length = sizeof(s) - 1, .data = (uint8_t *)s}
#define str8_comp(s)       {sizeof(s) - 1, (uint8_t *)s}

typedef struct {
	str8       *data;
	VisDACount  count;
	VisDACount  capacity;
} str8_list;

typedef enum {
	IntegerConversionResult_Invalid,
	IntegerConversionResult_OutOfRange,
	IntegerConversionResult_Success,
} IntegerConversionResult;

typedef enum {
	IntegerConversionFlag_NoAutoHex = (1 << 0),
	IntegerConversionFlag_ForceHex  = (1 << 1),
} IntegerConversionFlags;

typedef struct {
	IntegerConversionResult result;
	union {
		uint64_t U64;
		int64_t  S64;
	} as;
	str8 unparsed;
} IntegerConversion;

static u8
utf8_sequence_length(u32 cp)
{
	u8 result = (cp >= 0x10000) +
	            (cp >= 0x00800) +
	            (cp >= 0x00080) +
	            1;
	return result;
}

static u32
utf8_encode(u8 out[4], u32 cp)
{
	u32 result;
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

static u64
round_up_to(u64 value, u64 multiple)
{
	u64 result = value;
	if (value % multiple != 0)
		result += multiple - value % multiple;
	return result;
}

#endif /* UTIL_H */
