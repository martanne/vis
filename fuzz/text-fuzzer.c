#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "fuzzer.h"
#include "text.h"
#include "text-util.h"
#include "util.h"

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

typedef enum CmdStatus (*Cmd)(Text *txt, const char *cmd);

static Mark mark = EMARK;

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
