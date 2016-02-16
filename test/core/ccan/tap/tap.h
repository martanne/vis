#ifndef CCAN_TAP_H
#define CCAN_TAP_H
/*-
 * Copyright (c) 2004 Nik Clayton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <ccan/compiler/compiler.h>

/**
 * plan_tests - announce the number of tests you plan to run
 * @tests: the number of tests
 *
 * This should be the first call in your test program: it allows tracing
 * of failures which mean that not all tests are run.
 *
 * If you don't know how many tests will actually be run, assume all of them
 * and use skip() if you don't actually run some tests.
 *
 * Example:
 *	plan_tests(13);
 */
void plan_tests(unsigned int tests);

/**
 * ok1 - Simple conditional test
 * @e: the expression which we expect to be true.
 *
 * This is the simplest kind of test: if the expression is true, the
 * test passes.  The name of the test which is printed will simply be
 * file name, line number, and the expression itself.
 *
 * Example:
 *	ok1(somefunc() == 1);
 */
# define ok1(e) ((e) ?							\
		 _gen_result(1, __func__, __FILE__, __LINE__, "%s", #e) : \
		 _gen_result(0, __func__, __FILE__, __LINE__, "%s", #e))

/**
 * ok - Conditional test with a name
 * @e: the expression which we expect to be true.
 * @...: the printf-style name of the test.
 *
 * If the expression is true, the test passes.  The name of the test will be
 * the filename, line number, and the printf-style string.  This can be clearer
 * than simply the expression itself.
 *
 * Example:
 *	ok1(somefunc() == 1);
 *	ok(somefunc() == 0, "Second somefunc() should fail");
 */
# define ok(e, ...) ((e) ?						\
		     _gen_result(1, __func__, __FILE__, __LINE__,	\
				 __VA_ARGS__) :				\
		     _gen_result(0, __func__, __FILE__, __LINE__,	\
				 __VA_ARGS__))

/**
 * pass - Note that a test passed
 * @...: the printf-style name of the test.
 *
 * For complicated code paths, it can be easiest to simply call pass() in one
 * branch and fail() in another.
 *
 * Example:
 *	int x = somefunc();
 *	if (x > 0)
 *		pass("somefunc() returned a valid value");
 *	else
 *		fail("somefunc() returned an invalid value");
 */
# define pass(...) ok(1, __VA_ARGS__)

/**
 * fail - Note that a test failed
 * @...: the printf-style name of the test.
 *
 * For complicated code paths, it can be easiest to simply call pass() in one
 * branch and fail() in another.
 */
# define fail(...) ok(0, __VA_ARGS__)

/* I don't find these to be useful. */
# define skip_if(cond, n, ...)				\
	if (cond) skip((n), __VA_ARGS__);		\
	else

# define skip_start(test, n, ...)			\
	do {						\
		if((test)) {				\
			skip(n,  __VA_ARGS__);		\
			continue;			\
		}

# define skip_end } while(0)

unsigned int _gen_result(int, const char *, const char *, unsigned int,
   const char *, ...) PRINTF_FMT(5, 6);

/**
 * diag - print a diagnostic message (use instead of printf/fprintf)
 * @fmt: the format of the printf-style message
 *
 * diag ensures that the output will not be considered to be a test
 * result by the TAP test harness.  It will append '\n' for you.
 *
 * Example:
 *	diag("Now running complex tests");
 */
void diag(const char *fmt, ...) PRINTF_FMT(1, 2);

/**
 * skip - print a diagnostic message (use instead of printf/fprintf)
 * @n: number of tests you're skipping.
 * @fmt: the format of the reason you're skipping the tests.
 *
 * Sometimes tests cannot be run because the test system lacks some feature:
 * you should explicitly document that you're skipping tests using skip().
 *
 * From the Test::More documentation:
 *   If it's something the user might not be able to do, use SKIP.  This
 *   includes optional modules that aren't installed, running under an OS that
 *   doesn't have some feature (like fork() or symlinks), or maybe you need an
 *   Internet connection and one isn't available.
 *
 * Example:
 *	#ifdef HAVE_SOME_FEATURE
 *	ok1(somefunc());
 *	#else
 *	skip(1, "Don't have SOME_FEATURE");
 *	#endif
 */
void skip(unsigned int n, const char *fmt, ...) PRINTF_FMT(2, 3);

/**
 * todo_start - mark tests that you expect to fail.
 * @fmt: the reason they currently fail.
 *
 * It's extremely useful to write tests before you implement the matching fix
 * or features: surround these tests by todo_start()/todo_end().  These tests
 * will still be run, but with additional output that indicates that they are
 * expected to fail.
 *
 * This way, should a test start to succeed unexpectedly, tools like prove(1)
 * will indicate this and you can move the test out of the todo block.  This
 * is much more useful than simply commenting out (or '#if 0') the tests.
 *
 * From the Test::More documentation:
 *   If it's something the programmer hasn't done yet, use TODO.  This is for
 *   any code you haven't written yet, or bugs you have yet to fix, but want to
 *   put tests in your testing script (always a good idea).
 *
 * Example:
 * static bool dwim(void)
 * {
 *	return false; // NYI
 * }
 * ...
 *	todo_start("dwim() not returning true yet");
 *	ok(dwim(), "Did what the user wanted");
 *	todo_end();
 */
void todo_start(const char *fmt, ...) PRINTF_FMT(1, 2);

/**
 * todo_end - end of tests you expect to fail.
 *
 * See todo_start().
 */
void todo_end(void);

/**
 * exit_status - the value that main should return.
 *
 * For maximum compatibility your test program should return a particular exit
 * code (ie. 0 if all tests were run, and every test which was expected to
 * succeed succeeded).
 *
 * Example:
 *	exit(exit_status());
 */
int exit_status(void);

/**
 * plan_no_plan - I have no idea how many tests I'm going to run.
 *
 * In some situations you may not know how many tests you will be running, or
 * you are developing your test program, and do not want to update the
 * plan_tests() call every time you make a change.  For those situations use
 * plan_no_plan() instead of plan_tests().  It indicates to the test harness
 * that an indeterminate number of tests will be run.
 *
 * Remember, if you fail to plan, you plan to fail.
 *
 * Example:
 *	plan_no_plan();
 *	while (random() % 2)
 *		ok1(somefunc());
 *	exit(exit_status());
 */
void plan_no_plan(void);

/**
 * plan_skip_all - Indicate that you will skip all tests.
 * @reason: the string indicating why you can't run any tests.
 *
 * If your test program detects at run time that some required functionality
 * is missing (for example, it relies on a database connection which is not
 * present, or a particular configuration option that has not been included
 * in the running kernel) use plan_skip_all() instead of plan_tests().
 *
 * Example:
 *	#ifndef HAVE_SOME_FEATURE
 *	plan_skip_all("Need SOME_FEATURE support");
 *	exit(exit_status());
 *	#else
 *	plan_tests(13);
 *	...
 *	#endif
 */
void plan_skip_all(const char *reason);

/**
 * tap_fail_callback - function to call when we fail
 *
 * This can be used to ease debugging, or exit on the first failure.
 */
void (*tap_fail_callback)(void);

#endif /* CCAN_TAP_H */
