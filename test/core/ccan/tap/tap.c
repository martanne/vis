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
#include "config.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "tap.h"

static int no_plan = 0;
static int skip_all = 0;
static int have_plan = 0;
static unsigned int test_count = 0; /* Number of tests that have been run */
static unsigned int e_tests = 0; /* Expected number of tests to run */
static unsigned int failures = 0; /* Number of tests that failed */
static char *todo_msg = NULL;
static const char *todo_msg_fixed = "libtap malloc issue";
static int todo = 0;
static int test_died = 0;
static int test_pid;

/* Encapsulate the pthread code in a conditional.  In the absence of
   libpthread the code does nothing.

   If you have multiple threads calling ok() etc. at the same time you would
   need this, but in that case your test numbers will be random and I'm not
   sure it makes sense. --RR
*/
#ifdef WANT_PTHREAD
#include <pthread.h>
static pthread_mutex_t M = PTHREAD_MUTEX_INITIALIZER;
# define LOCK pthread_mutex_lock(&M)
# define UNLOCK pthread_mutex_unlock(&M)
#else
# define LOCK
# define UNLOCK
#endif

static void
_expected_tests(unsigned int tests)
{

	printf("1..%d\n", tests);
	e_tests = tests;
}

static void
diagv(const char *fmt, va_list ap)
{
	fputs("# ", stdout);
	vfprintf(stdout, fmt, ap);
	fputs("\n", stdout);
}

static void
_diag(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
 	diagv(fmt, ap);
	va_end(ap);
}

/*
 * Generate a test result.
 *
 * ok -- boolean, indicates whether or not the test passed.
 * test_name -- the name of the test, may be NULL
 * test_comment -- a comment to print afterwards, may be NULL
 */
unsigned int
_gen_result(int ok, const char *func, const char *file, unsigned int line,
	    const char *test_name, ...)
{
	va_list ap;
	char *local_test_name = NULL;
	char *c;
	int name_is_digits;

	LOCK;

	test_count++;

	/* Start by taking the test name and performing any printf()
	   expansions on it */
	if(test_name != NULL) {
		va_start(ap, test_name);
		if (vasprintf(&local_test_name, test_name, ap) < 0)
			local_test_name = NULL;
		va_end(ap);

		/* Make sure the test name contains more than digits
		   and spaces.  Emit an error message and exit if it
		   does */
		if(local_test_name) {
			name_is_digits = 1;
			for(c = local_test_name; *c != '\0'; c++) {
				if(!isdigit((unsigned char)*c)
				   && !isspace((unsigned char)*c)) {
					name_is_digits = 0;
					break;
				}
			}

			if(name_is_digits) {
				_diag("    You named your test '%s'.  You shouldn't use numbers for your test names.", local_test_name);
				_diag("    Very confusing.");
			}
		}
	}

	if(!ok) {
		printf("not ");
		failures++;
	}

	printf("ok %d", test_count);

	if(test_name != NULL) {
		printf(" - ");

		/* Print the test name, escaping any '#' characters it
		   might contain */
		if(local_test_name != NULL) {
			flockfile(stdout);
			for(c = local_test_name; *c != '\0'; c++) {
				if(*c == '#')
					fputc('\\', stdout);
				fputc((int)*c, stdout);
			}
			funlockfile(stdout);
		} else {	/* vasprintf() failed, use a fixed message */
			printf("%s", todo_msg_fixed);
		}
	}

	/* If we're in a todo_start() block then flag the test as being
	   TODO.  todo_msg should contain the message to print at this
	   point.  If it's NULL then asprintf() failed, and we should
	   use the fixed message.

	   This is not counted as a failure, so decrement the counter if
	   the test failed. */
	if(todo) {
		printf(" # TODO %s", todo_msg ? todo_msg : todo_msg_fixed);
		if(!ok)
			failures--;
	}

	printf("\n");

	if(!ok)
		_diag("    Failed %stest (%s:%s() at line %d)",
		      todo ? "(TODO) " : "", file, func, line);

	free(local_test_name);

	UNLOCK;

	if (!ok && tap_fail_callback)
		tap_fail_callback();

	/* We only care (when testing) that ok is positive, but here we
	   specifically only want to return 1 or 0 */
	return ok ? 1 : 0;
}

/*
 * Cleanup at the end of the run, produce any final output that might be
 * required.
 */
static void
_cleanup(void)
{
	/* If we forked, don't do cleanup in child! */
	if (getpid() != test_pid)
		return;

	LOCK;

	/* If plan_no_plan() wasn't called, and we don't have a plan,
	   and we're not skipping everything, then something happened
	   before we could produce any output */
	if(!no_plan && !have_plan && !skip_all) {
		_diag("Looks like your test died before it could output anything.");
		UNLOCK;
		return;
	}

	if(test_died) {
		_diag("Looks like your test died just after %d.", test_count);
		UNLOCK;
		return;
	}


	/* No plan provided, but now we know how many tests were run, and can
	   print the header at the end */
	if(!skip_all && (no_plan || !have_plan)) {
		printf("1..%d\n", test_count);
	}

	if((have_plan && !no_plan) && e_tests < test_count) {
		_diag("Looks like you planned %d tests but ran %d extra.",
		      e_tests, test_count - e_tests);
		UNLOCK;
		return;
	}

	if((have_plan || !no_plan) && e_tests > test_count) {
		_diag("Looks like you planned %d tests but only ran %d.",
		      e_tests, test_count);
		if(failures) {
			_diag("Looks like you failed %d tests of %d run.",
			      failures, test_count);
		}
		UNLOCK;
		return;
	}

	if(failures)
		_diag("Looks like you failed %d tests of %d.",
		      failures, test_count);

	UNLOCK;
}

/*
 * Initialise the TAP library.  Will only do so once, however many times it's
 * called.
 */
static void
_tap_init(void)
{
	static int run_once = 0;

	if(!run_once) {
		test_pid = getpid();
		atexit(_cleanup);

		/* stdout needs to be unbuffered so that the output appears
		   in the same place relative to stderr output as it does
		   with Test::Harness */
//		setbuf(stdout, 0);
		run_once = 1;
	}
}

/*
 * Note that there's no plan.
 */
void
plan_no_plan(void)
{

	LOCK;

	_tap_init();

	if(have_plan != 0) {
		fprintf(stderr, "You tried to plan twice!\n");
		test_died = 1;
		UNLOCK;
		exit(255);
	}

	have_plan = 1;
	no_plan = 1;

	UNLOCK;
}

/*
 * Note that the plan is to skip all tests
 */
void
plan_skip_all(const char *reason)
{

	LOCK;

	_tap_init();

	skip_all = 1;

	printf("1..0");

	if(reason != NULL)
		printf(" # Skip %s", reason);

	printf("\n");

	UNLOCK;
}

/*
 * Note the number of tests that will be run.
 */
void
plan_tests(unsigned int tests)
{

	LOCK;

	_tap_init();

	if(have_plan != 0) {
		fprintf(stderr, "You tried to plan twice!\n");
		test_died = 1;
		UNLOCK;
		exit(255);
	}

	if(tests == 0) {
		fprintf(stderr, "You said to run 0 tests!  You've got to run something.\n");
		test_died = 1;
		UNLOCK;
		exit(255);
	}

	have_plan = 1;

	_expected_tests(tests);

	UNLOCK;
}

void
diag(const char *fmt, ...)
{
	va_list ap;

	LOCK;

	va_start(ap, fmt);
 	diagv(fmt, ap);
	va_end(ap);

	UNLOCK;
}

void
skip(unsigned int n, const char *fmt, ...)
{
	va_list ap;
	char *skip_msg;

	LOCK;

	va_start(ap, fmt);
	if (vasprintf(&skip_msg, fmt, ap) < 0)
		skip_msg = NULL;
	va_end(ap);

	while(n-- > 0) {
		test_count++;
		printf("ok %d # skip %s\n", test_count,
		       skip_msg != NULL ?
		       skip_msg : "libtap():malloc() failed");
	}

	free(skip_msg);

	UNLOCK;
}

void
todo_start(const char *fmt, ...)
{
	va_list ap;

	LOCK;

	va_start(ap, fmt);
	if (vasprintf(&todo_msg, fmt, ap) < 0)
		todo_msg = NULL;
	va_end(ap);

	todo = 1;

	UNLOCK;
}

void
todo_end(void)
{

	LOCK;

	todo = 0;
	free(todo_msg);

	UNLOCK;
}

static int
exit_status_(void)
{
	int r;

	LOCK;

	/* If there's no plan, just return the number of failures */
	if(no_plan || !have_plan) {
		UNLOCK;
		return failures;
	}

	/* Ran too many tests?  Return the number of tests that were run
	   that shouldn't have been */
	if(e_tests < test_count) {
		r = test_count - e_tests;
		UNLOCK;
		return r;
	}

	/* Return the number of tests that failed + the number of tests
	   that weren't run */
	r = failures + e_tests - test_count;
	UNLOCK;

	return r;
}

int
exit_status(void)
{
	int r = exit_status_();
	if (r > 255)
		r = 255;
	return r;
}
