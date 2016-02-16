/* We use the fact that pipes have a buffer greater than the size of
 * any output, and change stdout and stderr to use that.
 *
 * Since we don't use libtap for output, this looks like one big test. */
#include <ccan/tap/tap.h>
#include <ccan/tap/tap.c>
#include <stdio.h>
#include <limits.h>
#include <err.h>
#include <string.h>
#include <stdbool.h>
#include <fnmatch.h>



/* We dup stderr to here. */
static int stderrfd;

/* write_all inlined here to avoid circular dependency. */
static void write_all(int fd, const void *data, size_t size)
{
	while (size) {
		ssize_t done;

		done = write(fd, data, size);
		if (done <= 0)
			_exit(1);
		data = (const char *)data + done;
		size -= done;
	}
}

/* Simple replacement for err() */
static void failmsg(const char *fmt, ...)
{
 	char buf[1024];
	va_list ap;

	/* Write into buffer. */
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	write_all(stderrfd, "# ", 2);
	write_all(stderrfd, buf, strlen(buf));
	write_all(stderrfd, "\n", 1);
	_exit(1);
}

static void expect(int fd, const char *pattern)
{
 	char buffer[PIPE_BUF+1];
	int r;

	r = read(fd, buffer, sizeof(buffer)-1);
	if (r < 0)
		failmsg("reading from pipe");
	buffer[r] = '\0';

	if (fnmatch(pattern, buffer, 0) != 0)
		failmsg("Expected '%s' got '%s'", pattern, buffer);
}

int main(int argc, char *argv[])
{
	int p[2];
	int stdoutfd;

	setbuf(stdout, 0);
	printf("1..1\n");
	stderrfd = dup(STDERR_FILENO);
	if (stderrfd < 0)
		err(1, "dup of stderr failed");

	stdoutfd = dup(STDOUT_FILENO);
	if (stdoutfd < 0)
		err(1, "dup of stdout failed");

	if (pipe(p) != 0)
		failmsg("pipe failed");

	if (dup2(p[1], STDERR_FILENO) < 0 || dup2(p[1], STDOUT_FILENO) < 0)
		failmsg("Duplicating file descriptor");

	plan_tests(10);
	expect(p[0], "1..10\n");

	ok(1, "msg1");
	expect(p[0], "ok 1 - msg1\n");

	ok(0, "msg2");
	expect(p[0], "not ok 2 - msg2\n"
	       "#     Failed test (*test/run.c:main() at line 91)\n");

	ok1(true);
	expect(p[0], "ok 3 - true\n");

	ok1(false);
 	expect(p[0], "not ok 4 - false\n"
	       "#     Failed test (*test/run.c:main() at line 98)\n");

	pass("passed");
 	expect(p[0], "ok 5 - passed\n");

	fail("failed");
 	expect(p[0], "not ok 6 - failed\n"
	       "#     Failed test (*test/run.c:main() at line 105)\n");

	skip(2, "skipping %s", "test");
 	expect(p[0], "ok 7 # skip skipping test\n"
	       "ok 8 # skip skipping test\n");

	todo_start("todo");
	ok1(false);
	expect(p[0], "not ok 9 - false # TODO todo\n"
	       "#     Failed (TODO) test (*test/run.c:main() at line 114)\n");
	ok1(true);
	expect(p[0], "ok 10 - true # TODO todo\n");
	todo_end();

	if (exit_status() != 3)
		failmsg("Expected exit status 3, not %i", exit_status());

#if 0
	/* Manually run the atexit command. */
	_cleanup();
	expect(p[0], "# Looks like you failed 2 tests of 9.\n");
#endif

	write_all(stdoutfd, "ok 1 - All passed\n",
		  strlen("ok 1 - All passed\n"));
	exit(0);
}
