#ifndef TAP_H
#define TAP_H

#ifdef TIS_INTERPRETER
static int failures = 0;
static int test_count = 0;
static void plan_no_plan(void) { }

static int exit_status() {
	if (failures > 255)
		failures = 255;
	return failures;
} 

#define ok(e, ...) do { \
	bool _e = (e); \
	printf("%sok %d - ", _e ? "" : "not ", ++test_count); \
	printf(__VA_ARGS__); \
	printf("\n"); \
	if (!_e) { \
		failures++; \
		printf(" Failed test (%s:%s() at line %d)\n", __FILE__, __func__, __LINE__); \
	} \
} while (0)

#define skip_if(cond, n, ...)                          \
        if (cond) skip((n), __VA_ARGS__);              \
        else

#define skip(n, ...) do { \
	int _n = (n); \
	while (_n--) { \
		printf("ok %d # skip ", ++test_count); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#include <time.h>
time_t time(time_t *p)
{
	static time_t value;
	value++;
	if (p) *p = value;
	return value;
}

#else
#include <ccan/tap/tap.h>
#define TIS_INTERPRETER 0
#endif

#endif
