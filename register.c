#include <stdlib.h>
#include <string.h>

#include "vis.h"
#include "text.h"
#include "util.h"
#include "register.h"

typedef struct {
	Buffer *stdout;
	Buffer *stderr;
} Clipboard;

static ssize_t read_stdout(void *context, char *data, size_t len) {
	Buffer *buf = ((Clipboard*)context)->stdout;
	buffer_append(buf, data, len);
	return len;
}

static ssize_t read_stderr(void *context, char *data, size_t len) {
	Buffer *buf = ((Clipboard*)context)->stderr;
	buffer_append(buf, data, len);
	return len;
}

void register_release(Register *reg) {
	buffer_release(&reg->buf);
}

const char *register_get(Vis *vis, Register *reg, size_t *len) {
	switch (reg->type) {
	case REGISTER_NORMAL:
		if (reg->buf.len > 0 && reg->buf.data[reg->buf.len-1] != '\0')
			buffer_append(&reg->buf, "\0", 1);
		if (len)
			*len = reg->buf.len > 0 ? reg->buf.len - 1 : 0;
		return reg->buf.data;
	case REGISTER_CLIPBOARD:
	{
		Buffer stderr;
		buffer_init(&stderr);
		buffer_clear(&reg->buf);
		Clipboard clipboard = {
			.stdout = &reg->buf,
			.stderr = &stderr,
		};

		int status = vis_pipe(vis, &clipboard,
			&(Filerange){ .start = 0, .end = 0 },
			(const char*[]){ "vis-clipboard", "vis-clipboard", "--paste", NULL },
			read_stdout, read_stderr);
		if (status != 0)
			vis_info_show(vis, "Command failed %s", stderr.len > 0 ? stderr.data : "");
		*len = reg->buf.len;
		return reg->buf.data;
	}
	case REGISTER_BLACKHOLE:
	default:
		*len = 0;
		return NULL;
	}
}

bool register_put(Vis *vis, Register *reg, const char *data, size_t len) {
	return reg->type == REGISTER_NORMAL && buffer_put(&reg->buf, data, len);
}

bool register_put0(Vis *vis, Register *reg, const char *data) {
	return register_put(vis, reg, data, strlen(data)+1);
}

bool register_put_range(Vis *vis, Register *reg, Text *txt, Filerange *range) {
	if (reg->append)
		return register_append_range(reg, txt, range);
	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		size_t len = text_range_size(range);
		if (!buffer_grow(&reg->buf, len))
			return false;
		reg->buf.len = text_bytes_get(txt, range->start, len, reg->buf.data);
		return true;
	}
	case REGISTER_CLIPBOARD:
	{
		Buffer stderr;
		buffer_init(&stderr);
		Clipboard clipboard = {
			.stderr = &stderr,
		};

		int status = vis_pipe(vis, &clipboard, range,
			(const char*[]){ "vis-clipboard", "vis-clipboard", "--copy", NULL },
			NULL, read_stderr);

		if (status != 0)
			vis_info_show(vis, "Command failed %s", stderr.len > 0 ? stderr.data : "");
		return status == 0;
	}
	case REGISTER_BLACKHOLE:
		return true;
	default:
		return false;
	}
}

bool register_append_range(Register *reg, Text *txt, Filerange *range) {
	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		size_t len = text_range_size(range);
		if (!buffer_grow(&reg->buf, reg->buf.len + len))
			return false;
		reg->buf.len += text_bytes_get(txt, range->start, len, reg->buf.data + reg->buf.len);
		return true;
	}
	default:
		return false;
	}
}
