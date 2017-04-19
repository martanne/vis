#include <stdlib.h>
#include <string.h>

#include "vis-core.h"
#include "text.h"
#include "util.h"
#include "register.h"
#include "buffer.h"

static Buffer *register_buffer(Register *reg, size_t slot) {
	Buffer *buf = array_get(&reg->values, slot);
	if (buf)
		return buf;
	if (array_resize(&reg->values, slot) && (buf = array_get(&reg->values, slot)))
		return buf;
	Buffer new;
	buffer_init(&new);
	if (!array_add(&reg->values, &new))
		return NULL;
	size_t capacity = array_capacity(&reg->values);
	for (size_t i = array_length(&reg->values); i < capacity; i++) {
		if (!array_add(&reg->values, &new))
			return NULL;
	}
	return array_get(&reg->values, slot);
}

static ssize_t read_buffer(void *context, char *data, size_t len) {
	buffer_append(context, data, len);
	return len;
}

bool register_init(Register *reg) {
	Buffer buf;
	buffer_init(&buf);
	array_init_sized(&reg->values, sizeof(Buffer));
	return array_add(&reg->values, &buf);
}

void register_release(Register *reg) {
	if (!reg)
		return;
	size_t n = array_capacity(&reg->values);
	for (size_t i = 0; i < n; i++)
		buffer_release(array_get(&reg->values, i));
	array_release(&reg->values);
}

const char *register_slot_get(Vis *vis, Register *reg, size_t slot, size_t *len) {
	if (len)
		*len = 0;
	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		Buffer *buf = array_get(&reg->values, slot);
		if (!buf)
			return NULL;
		buffer_terminate(buf);
		if (len)
			*len = buffer_length0(buf);
		return buffer_content0(buf);
	}
	case REGISTER_CLIPBOARD:
	{
		Buffer buferr;
		buffer_init(&buferr);
		Buffer *buf = array_get(&reg->values, slot);
		if (!buf)
			return NULL;
		buffer_clear(buf);

		int status = vis_pipe(vis, vis->win->file,
			&(Filerange){ .start = 0, .end = 0 },
			(const char*[]){ VIS_CLIPBOARD, "--paste", NULL },
			buf, read_buffer, &buferr, read_buffer);

		if (status != 0)
			vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));
		buffer_release(&buferr);
		if (len)
			*len = buffer_length0(buf);
		return buffer_content0(buf);
	}
	case REGISTER_BLACKHOLE:
	default:
		return NULL;
	}
}

const char *register_get(Vis *vis, Register *reg, size_t *len) {
	return register_slot_get(vis, reg, 0, len);
}

bool register_slot_put(Vis *vis, Register *reg, size_t slot, const char *data, size_t len) {
	if (reg->type != REGISTER_NORMAL)
		return false;
	Buffer *buf = register_buffer(reg, slot);
	return buf && buffer_put(buf, data, len);
}

bool register_put(Vis *vis, Register *reg, const char *data, size_t len) {
	return register_slot_put(vis, reg, 0, data, len) &&
	       register_resize(reg, 1);
}

bool register_put0(Vis *vis, Register *reg, const char *data) {
	return register_put(vis, reg, data, strlen(data)+1);
}

static bool register_slot_append_range(Register *reg, size_t slot, Text *txt, Filerange *range) {
	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		Buffer *buf = register_buffer(reg, slot);
		if (!buf)
			return false;
		size_t len = text_range_size(range);
		if (len == SIZE_MAX || !buffer_grow(buf, len+1))
			return false;
		if (buf->len > 0 && buf->data[buf->len-1] == '\0')
			buf->len--;
		buf->len += text_bytes_get(txt, range->start, len, buf->data + buf->len);
		return buffer_append(buf, "\0", 1);
	}
	default:
		return false;
	}
}

bool register_slot_put_range(Vis *vis, Register *reg, size_t slot, Text *txt, Filerange *range) {
	if (reg->append)
		return register_slot_append_range(reg, slot, txt, range);

	switch (reg->type) {
	case REGISTER_NORMAL:
	{
		Buffer *buf = register_buffer(reg, slot);
		if (!buf)
			return false;
		size_t len = text_range_size(range);
		if (len == SIZE_MAX || !buffer_reserve(buf, len+1))
			return false;
		buf->len = text_bytes_get(txt, range->start, len, buf->data);
		return buffer_append(buf, "\0", 1);
	}
	case REGISTER_CLIPBOARD:
	{
		Buffer buferr;
		buffer_init(&buferr);

		int status = vis_pipe(vis, vis->win->file, range,
			(const char*[]){ VIS_CLIPBOARD, "--copy", NULL },
			NULL, NULL, &buferr, read_buffer);

		if (status != 0)
			vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));
		buffer_release(&buferr);
		return status == 0;
	}
	case REGISTER_BLACKHOLE:
		return true;
	default:
		return false;
	}
}

bool register_put_range(Vis *vis, Register *reg, Text *txt, Filerange *range) {
	return register_slot_put_range(vis, reg, 0, txt, range) &&
	       register_resize(reg, 1);
}

size_t register_count(Register *reg) {
	return array_length(&reg->values);
}

bool register_resize(Register *reg, size_t count) {
	return array_truncate(&reg->values, count);
}
