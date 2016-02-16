/* Simple tool to create config.h.
 * Would be much easier with ccan modules, but deliberately standalone.
 *
 * Copyright 2011 Rusty Russell <rusty@rustcorp.com.au>.  MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define DEFAULT_COMPILER "cc"
#define DEFAULT_FLAGS "-g3 -ggdb -Wall -Wundef -Wmissing-prototypes -Wmissing-declarations -Wstrict-prototypes -Wold-style-definition"

#define OUTPUT_FILE "configurator.out"
#define INPUT_FILE "configuratortest.c"

static int verbose;

enum test_style {
	OUTSIDE_MAIN		= 0x1,
	DEFINES_FUNC		= 0x2,
	INSIDE_MAIN		= 0x4,
	DEFINES_EVERYTHING	= 0x8,
	MAY_NOT_COMPILE		= 0x10,
	EXECUTE			= 0x8000
};

struct test {
	const char *name;
	enum test_style style;
	const char *depends;
	const char *link;
	const char *fragment;
	const char *overrides; /* On success, force this to '1' */
	bool done;
	bool answer;
};

static struct test tests[] = {
	{ "HAVE_32BIT_OFF_T", DEFINES_EVERYTHING|EXECUTE, NULL, NULL,
	  "#include <sys/types.h>\n"
	  "int main(int argc, char *argv[]) {\n"
	  "	return sizeof(off_t) == 4 ? 0 : 1;\n"
	  "}\n" },
	{ "HAVE_ALIGNOF", INSIDE_MAIN, NULL, NULL,
	  "return __alignof__(double) > 0 ? 0 : 1;" },
	{ "HAVE_ASPRINTF", DEFINES_FUNC, NULL, NULL,
	  "#define _GNU_SOURCE\n"
	  "#include <stdio.h>\n"
	  "static char *func(int x) {"
	  "	char *p;\n"
	  "	if (asprintf(&p, \"%u\", x) == -1) p = NULL;"
	  "	return p;\n"
	  "}" },
	{ "HAVE_ATTRIBUTE_COLD", DEFINES_FUNC, NULL, NULL,
	  "static int __attribute__((cold)) func(int x) { return x; }" },
	{ "HAVE_ATTRIBUTE_CONST", DEFINES_FUNC, NULL, NULL,
	  "static int __attribute__((const)) func(int x) { return x; }" },
	{ "HAVE_ATTRIBUTE_PURE", DEFINES_FUNC, NULL, NULL,
	  "static int __attribute__((pure)) func(int x) { return x; }" },
	{ "HAVE_ATTRIBUTE_MAY_ALIAS", OUTSIDE_MAIN, NULL, NULL,
	  "typedef short __attribute__((__may_alias__)) short_a;" },
	{ "HAVE_ATTRIBUTE_NORETURN", DEFINES_FUNC, NULL, NULL,
	  "#include <stdlib.h>\n"
	  "static void __attribute__((noreturn)) func(int x) { exit(x); }" },
	{ "HAVE_ATTRIBUTE_PRINTF", DEFINES_FUNC, NULL, NULL,
	  "static void __attribute__((format(__printf__, 1, 2))) func(const char *fmt, ...) { }" },
	{ "HAVE_ATTRIBUTE_UNUSED", OUTSIDE_MAIN, NULL, NULL,
	  "static int __attribute__((unused)) func(int x) { return x; }" },
	{ "HAVE_ATTRIBUTE_USED", OUTSIDE_MAIN, NULL, NULL,
	  "static int __attribute__((used)) func(int x) { return x; }" },
	{ "HAVE_BACKTRACE", DEFINES_FUNC, NULL, NULL,
	  "#include <execinfo.h>\n"
	  "static int func(int x) {"
	  "	void *bt[10];\n"
	  "	return backtrace(bt, 10) < x;\n"
	  "}" },
	{ "HAVE_BIG_ENDIAN", INSIDE_MAIN|EXECUTE, NULL, NULL,
	  "union { int i; char c[sizeof(int)]; } u;\n"
	  "u.i = 0x01020304;\n"
	  "return u.c[0] == 0x01 && u.c[1] == 0x02 && u.c[2] == 0x03 && u.c[3] == 0x04 ? 0 : 1;" },
	{ "HAVE_BSWAP_64", DEFINES_FUNC, "HAVE_BYTESWAP_H", NULL,
	  "#include <byteswap.h>\n"
	  "static int func(int x) { return bswap_64(x); }" },
	{ "HAVE_BUILTIN_CHOOSE_EXPR", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_choose_expr(1, 0, \"garbage\");" },
	{ "HAVE_BUILTIN_CLZ", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_clz(1) == (sizeof(int)*8 - 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_CLZL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_clzl(1) == (sizeof(long)*8 - 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_CLZLL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_clzll(1) == (sizeof(long long)*8 - 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_CTZ", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_ctz(1 << (sizeof(int)*8 - 1)) == (sizeof(int)*8 - 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_CTZL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_ctzl(1UL << (sizeof(long)*8 - 1)) == (sizeof(long)*8 - 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_CTZLL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_ctzll(1ULL << (sizeof(long long)*8 - 1)) == (sizeof(long long)*8 - 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_CONSTANT_P", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_constant_p(1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_EXPECT", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_expect(argc == 1, 1) ? 0 : 1;" },
	{ "HAVE_BUILTIN_FFS", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_ffs(0) == 0 ? 0 : 1;" },
	{ "HAVE_BUILTIN_FFSL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_ffsl(0L) == 0 ? 0 : 1;" },
	{ "HAVE_BUILTIN_FFSLL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_ffsll(0LL) == 0 ? 0 : 1;" },
	{ "HAVE_BUILTIN_POPCOUNTL", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_popcountl(255L) == 8 ? 0 : 1;" },
	{ "HAVE_BUILTIN_TYPES_COMPATIBLE_P", INSIDE_MAIN, NULL, NULL,
	  "return __builtin_types_compatible_p(char *, int) ? 1 : 0;" },
	{ "HAVE_ICCARM_INTRINSICS", DEFINES_FUNC, NULL, NULL,
	  "#include <intrinsics.h>\n"
	  "int func(int v) {\n"
	  "	return __CLZ(__RBIT(v));\n"
	  "}" },
	{ "HAVE_BYTESWAP_H", OUTSIDE_MAIN, NULL, NULL,
	  "#include <byteswap.h>\n" },
	{ "HAVE_CLOCK_GETTIME",
	  DEFINES_FUNC, "HAVE_STRUCT_TIMESPEC", NULL,
	  "#include <time.h>\n"
	  "static struct timespec func(void) {\n"
	  "	struct timespec ts;\n"
	  "	clock_gettime(CLOCK_REALTIME, &ts);\n"
	  "	return ts;\n"
	  "}\n" },
	{ "HAVE_CLOCK_GETTIME_IN_LIBRT",
	  DEFINES_FUNC,
	  "HAVE_STRUCT_TIMESPEC !HAVE_CLOCK_GETTIME",
	  "-lrt",
	  "#include <time.h>\n"
	  "static struct timespec func(void) {\n"
	  "	struct timespec ts;\n"
	  "	clock_gettime(CLOCK_REALTIME, &ts);\n"
	  "	return ts;\n"
	  "}\n",
	  /* This means HAVE_CLOCK_GETTIME, too */
	  "HAVE_CLOCK_GETTIME" },
	{ "HAVE_COMPOUND_LITERALS", INSIDE_MAIN, NULL, NULL,
	  "int *foo = (int[]) { 1, 2, 3, 4 };\n"
	  "return foo[0] ? 0 : 1;" },
	{ "HAVE_FCHDIR", DEFINES_EVERYTHING|EXECUTE, NULL, NULL,
	  "#include <sys/types.h>\n"
	  "#include <sys/stat.h>\n"
	  "#include <fcntl.h>\n"
	  "#include <unistd.h>\n"
	  "int main(void) {\n"
	  "	int fd = open(\"..\", O_RDONLY);\n"
	  "	return fchdir(fd) == 0 ? 0 : 1;\n"
	  "}\n" },
	{ "HAVE_ERR_H", DEFINES_FUNC, NULL, NULL,
	  "#include <err.h>\n"
	  "static void func(int arg) {\n"
	  "	if (arg == 0)\n"
	  "		err(1, \"err %u\", arg);\n"
	  "	if (arg == 1)\n"
	  "		errx(1, \"err %u\", arg);\n"
	  "	if (arg == 3)\n"
	  "		warn(\"warn %u\", arg);\n"
	  "	if (arg == 4)\n"
	  "		warnx(\"warn %u\", arg);\n"
	  "}\n" },
	{ "HAVE_FILE_OFFSET_BITS", DEFINES_EVERYTHING|EXECUTE,
	  "HAVE_32BIT_OFF_T", NULL,
	  "#define _FILE_OFFSET_BITS 64\n"
	  "#include <sys/types.h>\n"
	  "int main(int argc, char *argv[]) {\n"
	  "	return sizeof(off_t) == 8 ? 0 : 1;\n"
	  "}\n" },
	{ "HAVE_FOR_LOOP_DECLARATION", INSIDE_MAIN, NULL, NULL,
	  "for (int i = 0; i < argc; i++) { return 0; };\n"
	  "return 1;" },
	{ "HAVE_FLEXIBLE_ARRAY_MEMBER", OUTSIDE_MAIN, NULL, NULL,
	  "struct foo { unsigned int x; int arr[]; };" },
	{ "HAVE_GETPAGESIZE", DEFINES_FUNC, NULL, NULL,
	  "#include <unistd.h>\n"
	  "static int func(void) { return getpagesize(); }" },
	{ "HAVE_ISBLANK", DEFINES_FUNC, NULL, NULL,
	  "#define _GNU_SOURCE\n"
	  "#include <ctype.h>\n"
	  "static int func(void) { return isblank(' '); }" },
	{ "HAVE_LITTLE_ENDIAN", INSIDE_MAIN|EXECUTE, NULL, NULL,
	  "union { int i; char c[sizeof(int)]; } u;\n"
	  "u.i = 0x01020304;\n"
	  "return u.c[0] == 0x04 && u.c[1] == 0x03 && u.c[2] == 0x02 && u.c[3] == 0x01 ? 0 : 1;" },
	{ "HAVE_MEMMEM", DEFINES_FUNC, NULL, NULL,
	  "#define _GNU_SOURCE\n"
	  "#include <string.h>\n"
	  "static void *func(void *h, size_t hl, void *n, size_t nl) {\n"
	  "return memmem(h, hl, n, nl);"
	  "}\n", },
	{ "HAVE_MEMRCHR", DEFINES_FUNC, NULL, NULL,
	  "#define _GNU_SOURCE\n"
	  "#include <string.h>\n"
	  "static void *func(void *s, int c, size_t n) {\n"
	  "return memrchr(s, c, n);"
	  "}\n", },
	{ "HAVE_MMAP", DEFINES_FUNC, NULL, NULL,
	  "#include <sys/mman.h>\n"
	  "static void *func(int fd) {\n"
	  "	return mmap(0, 65536, PROT_READ, MAP_SHARED, fd, 0);\n"
	  "}" },
	{ "HAVE_PROC_SELF_MAPS", DEFINES_EVERYTHING|EXECUTE, NULL, NULL,
	  "#include <sys/types.h>\n"
	  "#include <sys/stat.h>\n"
	  "#include <fcntl.h>\n"
	  "int main(void) {\n"
	  "	return open(\"/proc/self/maps\", O_RDONLY) != -1 ? 0 : 1;\n"
	  "}\n" },
	{ "HAVE_QSORT_R_PRIVATE_LAST",
	  DEFINES_EVERYTHING|EXECUTE|MAY_NOT_COMPILE, NULL, NULL,
	  "#define _GNU_SOURCE 1\n"
	  "#include <stdlib.h>\n"
	  "static int cmp(const void *lp, const void *rp, void *priv) {\n"
	  " *(unsigned int *)priv = 1;\n"
	  " return *(const int *)lp - *(const int *)rp; }\n"
	  "int main(void) {\n"
	  " int array[] = { 9, 2, 5 };\n"
	  " unsigned int called = 0;\n"
	  " qsort_r(array, 3, sizeof(int), cmp, &called);\n"
	  " return called && array[0] == 2 && array[1] == 5 && array[2] == 9 ? 0 : 1;\n"
	  "}\n" },
	{ "HAVE_STRUCT_TIMESPEC",
	  DEFINES_FUNC, NULL, NULL,
	  "#include <time.h>\n"
	  "static void func(void) {\n"
	  "	struct timespec ts;\n"
	  "	ts.tv_sec = ts.tv_nsec = 1;\n"
	  "}\n" },
	{ "HAVE_SECTION_START_STOP",
	  DEFINES_FUNC, NULL, NULL,
	  "static void *__attribute__((__section__(\"mysec\"))) p = &p;\n"
	  "static int func(void) {\n"
	  "	extern void *__start_mysec[], *__stop_mysec[];\n"
	  "	return __stop_mysec - __start_mysec;\n"
	  "}\n" },
	{ "HAVE_STACK_GROWS_UPWARDS", DEFINES_EVERYTHING|EXECUTE, NULL, NULL,
	  "static long nest(const void *base, unsigned int i)\n"
	  "{\n"
	  "	if (i == 0)\n"
	  "		return (const char *)&i - (const char *)base;\n"
	  "	return nest(base, i-1);\n"
	  "}\n"
	  "int main(int argc, char *argv[]) {\n"
	  "	return (nest(&argc, argc) > 0) ? 0 : 1\n;"
	  "}\n" },
	{ "HAVE_STATEMENT_EXPR", INSIDE_MAIN, NULL, NULL,
	  "return ({ int x = argc; x == argc ? 0 : 1; });" },
	{ "HAVE_SYS_FILIO_H", OUTSIDE_MAIN, NULL, NULL, /* Solaris needs this for FIONREAD */
	  "#include <sys/filio.h>\n" },
	{ "HAVE_SYS_TERMIOS_H", OUTSIDE_MAIN, NULL, NULL,
	  "#include <sys/termios.h>\n" },
	{ "HAVE_TYPEOF", INSIDE_MAIN, NULL, NULL,
	  "__typeof__(argc) i; i = argc; return i == argc ? 0 : 1;" },
	{ "HAVE_UNALIGNED_ACCESS", DEFINES_EVERYTHING|EXECUTE, NULL, NULL,
	  "#include <string.h>\n"
	  "int main(int argc, char *argv[]) {\n"
	  "     char pad[sizeof(int *) * 1];\n"
	  "	strncpy(pad, argv[0], sizeof(pad));\n"
	  "	return *(int *)(pad) == *(int *)(pad + 1);\n"
	  "}\n" },
	{ "HAVE_UTIME", DEFINES_FUNC, NULL, NULL,
	  "#include <sys/types.h>\n"
	  "#include <utime.h>\n"
	  "static int func(const char *filename) {\n"
	  "	struct utimbuf times = { 0 };\n"
	  "	return utime(filename, &times);\n"
	  "}" },
	{ "HAVE_WARN_UNUSED_RESULT", DEFINES_FUNC, NULL, NULL,
	  "#include <sys/types.h>\n"
	  "#include <utime.h>\n"
	  "static __attribute__((warn_unused_result)) int func(int i) {\n"
	  "	return i + 1;\n"
	  "}" },
};

static char *grab_fd(int fd)
{
	int ret;
	size_t max, size = 0;
	char *buffer;

	max = 16384;
	buffer = malloc(max+1);
	while ((ret = read(fd, buffer + size, max - size)) > 0) {
		size += ret;
		if (size == max)
			buffer = realloc(buffer, max *= 2);
	}
	if (ret < 0)
		err(1, "reading from command");
	buffer[size] = '\0';
	return buffer;
}

static char *run(const char *cmd, int *exitstatus)
{
	pid_t pid;
	int p[2];
	char *ret;
	int status;

	if (pipe(p) != 0)
		err(1, "creating pipe");

	pid = fork();
	if (pid == -1)
		err(1, "forking");

	if (pid == 0) {
		if (dup2(p[1], STDOUT_FILENO) != STDOUT_FILENO
		    || dup2(p[1], STDERR_FILENO) != STDERR_FILENO
		    || close(p[0]) != 0
		    || close(STDIN_FILENO) != 0
		    || open("/dev/null", O_RDONLY) != STDIN_FILENO)
			exit(128);

		status = system(cmd);
		if (WIFEXITED(status))
			exit(WEXITSTATUS(status));
		/* Here's a hint... */
		exit(128 + WTERMSIG(status));
	}

	close(p[1]);
	ret = grab_fd(p[0]);
	/* This shouldn't fail... */
	if (waitpid(pid, &status, 0) != pid)
		err(1, "Failed to wait for child");
	close(p[0]);
	if (WIFEXITED(status))
		*exitstatus = WEXITSTATUS(status);
	else
		*exitstatus = -WTERMSIG(status);
	return ret;
}

static char *connect_args(const char *argv[], const char *extra)
{
	unsigned int i, len = strlen(extra) + 1;
	char *ret;

	for (i = 1; argv[i]; i++)
		len += 1 + strlen(argv[i]);

	ret = malloc(len);
	len = 0;
	for (i = 1; argv[i]; i++) {
		strcpy(ret + len, argv[i]);
		len += strlen(argv[i]);
		if (argv[i+1])
			ret[len++] = ' ';
	}
	strcpy(ret + len, extra);
	return ret;
}

static struct test *find_test(const char *name)
{
	unsigned int i;

	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		if (strcmp(tests[i].name, name) == 0)
			return &tests[i];
	}
	abort();
}

#define PRE_BOILERPLATE "/* Test program generated by configurator. */\n"
#define MAIN_START_BOILERPLATE "int main(int argc, char *argv[]) {\n"
#define USE_FUNC_BOILERPLATE "(void)func;\n"
#define MAIN_BODY_BOILERPLATE "return 0;\n"
#define MAIN_END_BOILERPLATE "}\n"

static bool run_test(const char *cmd, struct test *test)
{
	char *output;
	FILE *outf;
	int status;

	if (test->done)
		return test->answer;

	if (test->depends) {
		size_t len;
		const char *deps = test->depends;
		char *dep;

		/* Space-separated dependencies, could be ! for inverse. */
		while ((len = strcspn(deps, " "))) {
			bool positive = true;
			if (deps[len]) {
				dep = strdup(deps);
				dep[len] = '\0';
			} else {
				dep = (char *)deps;
			}

			if (dep[0] == '!') {
				dep++;
				positive = false;
			}
			if (run_test(cmd, find_test(dep)) != positive) {
				test->answer = false;
				test->done = true;
				return test->answer;
			}
			deps += len;
			deps += strspn(deps, " ");
		}
	}

	outf = fopen(INPUT_FILE, "w");
	if (!outf)
		err(1, "creating %s", INPUT_FILE);

	fprintf(outf, "%s", PRE_BOILERPLATE);
	switch (test->style & ~(EXECUTE|MAY_NOT_COMPILE)) {
	case INSIDE_MAIN:
		fprintf(outf, "%s", MAIN_START_BOILERPLATE);
		fprintf(outf, "%s", test->fragment);
		fprintf(outf, "%s", MAIN_END_BOILERPLATE);
		break;
	case OUTSIDE_MAIN:
		fprintf(outf, "%s", test->fragment);
		fprintf(outf, "%s", MAIN_START_BOILERPLATE);
		fprintf(outf, "%s", MAIN_BODY_BOILERPLATE);
		fprintf(outf, "%s", MAIN_END_BOILERPLATE);
		break;
	case DEFINES_FUNC:
		fprintf(outf, "%s", test->fragment);
		fprintf(outf, "%s", MAIN_START_BOILERPLATE);
		fprintf(outf, "%s", USE_FUNC_BOILERPLATE);
		fprintf(outf, "%s", MAIN_BODY_BOILERPLATE);
		fprintf(outf, "%s", MAIN_END_BOILERPLATE);
		break;
	case DEFINES_EVERYTHING:
		fprintf(outf, "%s", test->fragment);
		break;
	default:
		abort();

	}
	fclose(outf);

	if (verbose > 1)
		if (system("cat " INPUT_FILE) == -1);

	if (test->link) {
		char *newcmd;
		newcmd = malloc(strlen(cmd) + strlen(" ")
				+ strlen(test->link) + 1);
		sprintf(newcmd, "%s %s", cmd, test->link);
		if (verbose > 1)
			printf("Extra link line: %s", newcmd);
		cmd = newcmd;
	}

	output = run(cmd, &status);
	if (status != 0 || strstr(output, "warning")) {
		if (verbose)
			printf("Compile %s for %s, status %i: %s\n",
			       status ? "fail" : "warning",
			       test->name, status, output);
		if ((test->style & EXECUTE) && !(test->style & MAY_NOT_COMPILE))
			errx(1, "Test for %s did not compile:\n%s",
			     test->name, output);
		test->answer = false;
		free(output);
	} else {
		/* Compile succeeded. */
		free(output);
		/* We run INSIDE_MAIN tests for sanity checking. */
		if ((test->style & EXECUTE) || (test->style & INSIDE_MAIN)) {
			output = run("./" OUTPUT_FILE, &status);
			if (!(test->style & EXECUTE) && status != 0)
				errx(1, "Test for %s failed with %i:\n%s",
				     test->name, status, output);
			if (verbose && status)
				printf("%s exited %i\n", test->name, status);
			free(output);
		}
		test->answer = (status == 0);
	}
	test->done = true;

	if (test->answer && test->overrides) {
		struct test *override = find_test(test->overrides);
		override->done = true;
		override->answer = true;
	}
	return test->answer;
}

int main(int argc, const char *argv[])
{
	char *cmd;
	unsigned int i;
	const char *default_args[]
		= { "", DEFAULT_COMPILER, DEFAULT_FLAGS, NULL };

	if (argc > 1) {
		if (strcmp(argv[1], "--help") == 0) {
			printf("Usage: configurator [-v] [<compiler> <flags>...]\n"
			       "  <compiler> <flags> will have \"-o <outfile> <infile.c>\" appended\n"
			       "Default: %s %s\n",
			       DEFAULT_COMPILER, DEFAULT_FLAGS);
			exit(0);
		}
		if (strcmp(argv[1], "-v") == 0) {
			argc--;
			argv++;
			verbose = 1;
		} else if (strcmp(argv[1], "-vv") == 0) {
			argc--;
			argv++;
			verbose = 2;
		}
	}

	if (argc == 1)
		argv = default_args;

	cmd = connect_args(argv, " -o " OUTPUT_FILE " " INPUT_FILE);
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
		run_test(cmd, &tests[i]);

	unlink(OUTPUT_FILE);
	unlink(INPUT_FILE);

	printf("/* Generated by CCAN configurator */\n"
	       "#ifndef CCAN_CONFIG_H\n"
	       "#define CCAN_CONFIG_H\n");
	printf("#ifndef _GNU_SOURCE\n");
	printf("#define _GNU_SOURCE /* Always use GNU extensions. */\n");
	printf("#endif\n");
	printf("#define CCAN_COMPILER \"%s\"\n", argv[1]);
	printf("#define CCAN_CFLAGS \"%s\"\n\n", connect_args(argv+1, ""));
	/* This one implies "#include <ccan/..." works, eg. for tdb2.h */
	printf("#define HAVE_CCAN 1\n");
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++)
		printf("#define %s %u\n", tests[i].name, tests[i].answer);
	printf("#endif /* CCAN_CONFIG_H */\n");
	return 0;
}
