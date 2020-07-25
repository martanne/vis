#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include "fuzzer.h"
#include "text.h"
#include "text-util.h"
#include "util.h"

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

typedef enum CmdStatus (*Cmd)(Text *txt, const char *cmd);

static Mark mark = EMARK;

static char data[BUFSIZ];

static uint64_t bench(void) {
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
		return (uint64_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
	else
		return 0;
}

static size_t pos_start(Text *txt) {
	return 0;
}

static size_t pos_middle(Text *txt) {
	return text_size(txt) / 2;
}

static size_t pos_end(Text *txt) {
	return text_size(txt);
}

static size_t pos_random(Text *txt) {
	return rand() % (text_size(txt) + 1);
}

static size_t pos_prev(Text *txt) {
	static size_t pos = EPOS;
	size_t max = text_size(txt);
	if (pos > max)
		pos = max;
	return pos-- % (max + 1);
}

static size_t pos_next(Text *txt) {
	static size_t pos = 0;
	return pos++ % (text_size(txt) + 1);
}

static size_t pos_stripe(Text *txt) {
	static size_t pos = 0;
	return pos+=1024 % (text_size(txt) + 1);
}

static enum CmdStatus bench_insert(Text *txt, size_t pos, const char *cmd) {
	return text_insert(txt, pos, data, sizeof data);
}

static enum CmdStatus bench_delete(Text *txt, size_t pos, const char *cmd) {
	return text_delete(txt, pos, 1);
}

static enum CmdStatus bench_replace(Text *txt, size_t pos, const char *cmd) {
	text_delete(txt, pos, 1);
	text_insert(txt, pos, "-", 1);
	return CMD_OK;
}

static enum CmdStatus bench_mark(Text *txt, size_t pos, const char *cmd) {
	Mark mark = text_mark_set(txt, pos);
	if (mark == EMARK)
		return CMD_FAIL;
	if (text_mark_get(txt, mark) != pos)
		return CMD_FAIL;
	return CMD_OK;
}

static enum CmdStatus cmd_bench(Text *txt, const char *cmd) {

	static enum CmdStatus (*bench_cmd[])(Text*, size_t, const char*) = {
		['i'] = bench_insert,
		['d'] = bench_delete,
		['r'] = bench_replace,
		['m'] = bench_mark,
	};

	static size_t (*bench_pos[])(Text*) = {
		['^'] = pos_start,
		['|'] = pos_middle,
		['$'] = pos_end,
		['%'] = pos_random,
		['-'] = pos_prev,
		['+'] = pos_next,
		['~'] = pos_stripe,
	};

	if (!data[0]) {
		// make `p` command output more readable
		int len = snprintf(data, sizeof data, "[ ... %zu bytes ... ]\n", sizeof data);
		memset(data+len, '\r', sizeof(data) - len);
	}

	const char *params = cmd;
	while (*params == ' ')
		params++;

	size_t idx_cmd = params[0];
	if (idx_cmd >= LENGTH(bench_cmd) || !bench_cmd[idx_cmd]) {
		puts("Invalid bench command");
		return CMD_ERR;
	}

	for (params++; *params == ' '; params++);

	size_t idx_pos = params[0];
	if (idx_pos >= LENGTH(bench_pos) || !bench_pos[idx_pos]) {
		puts("Invalid bench position");
		return CMD_ERR;
	}

	size_t iter = 1;
	sscanf(params+1, "%zu\n", &iter);

	for (size_t i = 1; i <= iter; i++) {
		size_t pos = bench_pos[idx_pos](txt);
		uint64_t s = bench();
		enum CmdStatus ret = bench_cmd[idx_cmd](txt, pos, NULL);
		uint64_t e = bench();
		if (ret != CMD_OK)
			return ret;
		printf("%zu: %" PRIu64 "us\n", i, e-s);
	}
	return CMD_OK;
}

static enum CmdStatus cmd_insert(Text *txt, const char *cmd) {
	char data[BUFSIZ];
	size_t pos;
	if (sscanf(cmd, "%zu %s\n", &pos, data) != 2)
		return CMD_ERR;
	size_t len = strlen(data);
	return text_insert(txt, pos, data, len);
}

static enum CmdStatus cmd_delete(Text *txt, const char *cmd) {
	size_t pos, len;
	if (sscanf(cmd, "%zu %zu", &pos, &len) != 2)
		return CMD_ERR;
	return text_delete(txt, pos, len);
}

static enum CmdStatus cmd_size(Text *txt, const char *cmd) {
	printf("%zu bytes\n", text_size(txt));
	return CMD_OK;
}

static enum CmdStatus cmd_snapshot(Text *txt, const char *cmd) {
	text_snapshot(txt);
	return CMD_OK;
}

static enum CmdStatus cmd_undo(Text *txt, const char *cmd) {
	return text_undo(txt) != EPOS;
}

static enum CmdStatus cmd_redo(Text *txt, const char *cmd) {
	return text_redo(txt) != EPOS;
}

static enum CmdStatus cmd_earlier(Text *txt, const char *cmd) {
	return text_earlier(txt) != EPOS;
}

static enum CmdStatus cmd_later(Text *txt, const char *cmd) {
	return text_later(txt) != EPOS;
}

static enum CmdStatus cmd_mark_set(Text *txt, const char *cmd) {
	size_t pos;
	if (sscanf(cmd, "%zu\n", &pos) != 1)
		return CMD_ERR;
	Mark m = text_mark_set(txt, pos);
	if (m != EMARK)
		mark = m;
	return m != EMARK;
}

static enum CmdStatus cmd_mark_get(Text *txt, const char *cmd) {
	size_t pos = text_mark_get(txt, mark);
	if (pos != EPOS)
		printf("%zu\n", pos);
	return pos != EPOS;
}

static enum CmdStatus cmd_print(Text *txt, const char *cmd) {
	size_t start = 0, size = text_size(txt), rem = size;
	for (Iterator it = text_iterator_get(txt, start);
	     rem > 0 && text_iterator_valid(&it);
	     text_iterator_next(&it)) {
		size_t prem = it.end - it.text;
		if (prem > rem)
			prem = rem;
		if (fwrite(it.text, prem, 1, stdout) != 1)
			return CMD_ERR;
		rem -= prem;
	}
	if (rem != size)
		puts("");
	return rem == 0; 
}

static enum CmdStatus cmd_quit(Text *txt, const char *cmd) {
	return CMD_QUIT;
}

static Cmd commands[] = {
	['-'] = cmd_earlier,
	['+'] = cmd_later,
	['?'] = cmd_mark_get,
	['='] = cmd_mark_set,
	['#'] = cmd_size,
	['b'] = cmd_bench,
	['d'] = cmd_delete,
	['i'] = cmd_insert,
	['p'] = cmd_print,
	['q'] = cmd_quit,
	['r'] = cmd_redo,
	['s'] = cmd_snapshot,
	['u'] = cmd_undo,
};

static int repl(const char *name, FILE *input) {
	Text *txt = text_load(name);
	if (!name)
		name = "-";
	if (!txt) {
		fprintf(stderr, "Failed to load text from `%s'\n", name);
		return 1;
	}

	printf("Loaded %zu bytes from `%s'\n", text_size(txt), name);

	char line[BUFSIZ];
	for (;;) {
		printf("> ");
		if (!fgets(line, sizeof(line), input))
			break;
		if (!isatty(0))
			printf("%s", line);
		if (line[0] == '\n')
			continue;
		size_t idx = line[0];
		if (idx < LENGTH(commands) && commands[idx]) {
			enum CmdStatus ret = commands[idx](txt, line+1);
			printf("%s", cmd_status_msg[ret]);
			if (ret == CMD_QUIT)
				break;
		} else {
			puts("Invalid command");
		}
	}

	text_free(txt);

	return 0;
}

#ifdef LIBFUZZER

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t len) {
	FILE *input = fmemopen((void*)data, len, "r");
	if (!input)
		return 1;
	int r = repl(NULL, input);
	fclose(input);
	return r;
}

#else

int main(int argc, char *argv[]) {
	return repl(argc == 1 ? NULL : argv[1], stdin);
}

#endif /* LIBFUZZER */
