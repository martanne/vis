#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "fuzzer.h"
#include "buffer.h"
#include "util.h"

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

typedef enum CmdStatus (*Cmd)(Buffer *buf, const char *cmd);

static enum CmdStatus cmd_insert(Buffer *buf, const char *cmd) {
	char data[BUFSIZ];
	size_t pos;
	if (sscanf(cmd, "%zu %s\n", &pos, data) != 2)
		return CMD_ERR;
	return buffer_insert0(buf, pos, data);
}

static enum CmdStatus cmd_set(Buffer *buf, const char *cmd) {
	char data[BUFSIZ];
	if (sscanf(cmd, "%s\n", data) != 1)
		return CMD_ERR;
	return buffer_put0(buf, data);
}

static enum CmdStatus cmd_delete(Buffer *buf, const char *cmd) {
	size_t pos, len;
	if (sscanf(cmd, "%zu %zu", &pos, &len) != 2)
		return CMD_ERR;
	return buffer_remove(buf, pos, len);
}

static enum CmdStatus cmd_clear(Buffer *buf, const char *cmd) {
	buf->len = 0;
	return CMD_OK;
}

static enum CmdStatus cmd_size(Buffer *buf, const char *cmd) {
	printf("%zu bytes\n", buffer_length0(buf));
	return CMD_OK;
}

static enum CmdStatus cmd_capacity(Buffer *buf, const char *cmd) {
	printf("%zu bytes\n", buf->size);
	return CMD_OK;
}

static enum CmdStatus cmd_print(Buffer *buf, const char *cmd) {
	size_t len = buffer_length0(buf);
	const char *data = buffer_content0(buf);
	if (data && fwrite(data, len, 1, stdout) != 1)
		return CMD_ERR;
	if (data)
		puts("");
	return CMD_OK;
}

static enum CmdStatus cmd_quit(Buffer *buf, const char *cmd) {
	return CMD_QUIT;
}

static Cmd commands[] = {
	['?'] = cmd_capacity,
	['='] = cmd_set,
	['#'] = cmd_size,
	['c'] = cmd_clear,
	['d'] = cmd_delete,
	['i'] = cmd_insert,
	['p'] = cmd_print,
	['q'] = cmd_quit,
};

int main(int argc, char *argv[]) {
	char line[BUFSIZ];
	Buffer buf = {0};

	for (;;) {
		printf("> ");
		if (!fgets(line, sizeof(line), stdin))
			break;
		if (!isatty(0))
			printf("%s", line);
		if (line[0] == '\n')
			continue;
		size_t idx = line[0];
		if (idx < LENGTH(commands) && commands[idx]) {
			enum CmdStatus ret = commands[idx](&buf, line+1);
			printf("%s", cmd_status_msg[ret]);
			if (ret == CMD_QUIT)
				break;
		} else {
			puts("Invalid command");
		}
	}

	buffer_release(&buf);

	return 0;
}
