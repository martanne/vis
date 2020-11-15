#include <stdlib.h>
#include <string.h>

#include "vis-core.h"

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
	case REGISTER_NUMBER:
	{
		Buffer *buf = array_get(&reg->values, 0);
		if (!buf)
			return NULL;
		buffer_printf(buf, "%zu", slot+1);
		if (len)
			*len = buffer_length0(buf);
		return buffer_content0(buf);
	}
	case REGISTER_CLIPBOARD:
	{
		Buffer buferr;
		enum VisRegister id = reg - vis->registers;
		const char *cmd[] = { VIS_CLIPBOARD, "--paste", "--selection", NULL, NULL };
		buffer_init(&buferr);
		Buffer *buf = array_get(&reg->values, slot);
		if (!buf)
			return NULL;
		buffer_clear(buf);

		if (id == VIS_REG_PRIMARY)
			cmd[3] = "primary";
		else
			cmd[3] = "clipboard";
		int status = vis_pipe(vis, vis->win->file,
			&(Filerange){ .start = 0, .end = 0 },
			cmd, buf, read_buffer, &buferr, read_buffer);

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
		const char *cmd[] = { VIS_CLIPBOARD, "--copy", "--selection", NULL, NULL };
		enum VisRegister id = reg - vis->registers;
		buffer_init(&buferr);

		if (id == VIS_REG_PRIMARY)
			cmd[3] = "primary";
		else
			cmd[3] = "clipboard";

		int status = vis_pipe(vis, vis->win->file, range,
			cmd, NULL, NULL, &buferr, read_buffer);

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

size_t vis_register_count(Vis *vis, Register *reg) {
	if (reg->type == REGISTER_NUMBER)
		return vis->win ? view_selections_count(vis->win->view) : 0;
	return array_length(&reg->values);
}

bool register_resize(Register *reg, size_t count) {
	return array_truncate(&reg->values, count);
}

enum VisRegister vis_register_from(Vis *vis, char reg) {

	if (reg == '@')
		return VIS_MACRO_LAST_RECORDED;

	if ('a' <= reg && reg <= 'z')
		return VIS_REG_a + reg - 'a';
	if ('A' <= reg && reg <= 'Z')
		return VIS_REG_A + reg - 'A';

	for (size_t i = 0; i < LENGTH(vis_registers); i++) {
		if (vis_registers[i].name == reg)
			return i;
	}
	return VIS_REG_INVALID;
}

char vis_register_to(Vis *vis, enum VisRegister reg) {

	if (reg == VIS_MACRO_LAST_RECORDED)
		return '@';

	if (VIS_REG_a <= reg && reg <= VIS_REG_z)
		return 'a' + reg - VIS_REG_a;
	if (VIS_REG_A <= reg && reg <= VIS_REG_Z)
		return 'A' + reg - VIS_REG_A;

	if (reg < LENGTH(vis_registers))
		return vis_registers[reg].name;

	return '\0';
}

void vis_register(Vis *vis, enum VisRegister reg) {
	if (VIS_REG_A <= reg && reg <= VIS_REG_Z) {
		vis->action.reg = &vis->registers[VIS_REG_a + reg - VIS_REG_A];
		vis->action.reg->append = true;
	} else if (reg < LENGTH(vis->registers)) {
		vis->action.reg = &vis->registers[reg];
		vis->action.reg->append = false;
	}
}

enum VisRegister vis_register_used(Vis *vis) {
	if (!vis->action.reg)
		return VIS_REG_DEFAULT;
	return vis->action.reg - vis->registers;
}

static Register *register_from(Vis *vis, enum VisRegister id) {
	if (VIS_REG_A <= id && id <= VIS_REG_Z)
		id = VIS_REG_a + id - VIS_REG_A;
	if (id < LENGTH(vis->registers))
		return &vis->registers[id];
	return NULL;
}

bool vis_register_set(Vis *vis, enum VisRegister id, Array *data) {
	Register *reg = register_from(vis, id);
	if (!reg)
		return false;
	size_t len = array_length(data);
	for (size_t i = 0; i < len; i++) {
		Buffer *buf = register_buffer(reg, i);
		if (!buf)
			return false;
		TextString *string = array_get(data, i);
		if (!buffer_put(buf, string->data, string->len))
			return false;
	}
	return register_resize(reg, len);
}

Array vis_register_get(Vis *vis, enum VisRegister id) {
	Array data;
	array_init_sized(&data, sizeof(TextString));
	Register *reg = register_from(vis, id);
	if (reg) {
		size_t len = array_length(&reg->values);
		array_reserve(&data, len);
		for (size_t i = 0; i < len; i++) {
			Buffer *buf = array_get(&reg->values, i);
			TextString string = {
				.data = buffer_content(buf),
				.len = buffer_length(buf),
			};
			array_add(&data, &string);
		}
	}
	return data;
}

const RegisterDef vis_registers[] = {
	[VIS_REG_DEFAULT]    = { '"', VIS_HELP("Unnamed register")                                 },
	[VIS_REG_ZERO]       = { '0', VIS_HELP("Yank register")                                    },
	[VIS_REG_1]          = { '1', VIS_HELP("1st sub-expression match")                         },
	[VIS_REG_2]          = { '2', VIS_HELP("2nd sub-expression match")                         },
	[VIS_REG_3]          = { '3', VIS_HELP("3rd sub-expression match")                         },
	[VIS_REG_4]          = { '4', VIS_HELP("4th sub-expression match")                         },
	[VIS_REG_5]          = { '5', VIS_HELP("5th sub-expression match")                         },
	[VIS_REG_6]          = { '6', VIS_HELP("6th sub-expression match")                         },
	[VIS_REG_7]          = { '7', VIS_HELP("7th sub-expression match")                         },
	[VIS_REG_8]          = { '8', VIS_HELP("8th sub-expression match")                         },
	[VIS_REG_9]          = { '9', VIS_HELP("9th sub-expression match")                         },
	[VIS_REG_AMPERSAND]  = { '&', VIS_HELP("Last regex match")                                 },
	[VIS_REG_BLACKHOLE]  = { '_', VIS_HELP("/dev/null register")                               },
	[VIS_REG_PRIMARY]    = { '*', VIS_HELP("Primary clipboard register, see vis-clipboard(1)") },
	[VIS_REG_CLIPBOARD]  = { '+', VIS_HELP("System clipboard register, see vis-clipboard(1)")  },
	[VIS_REG_DOT]        = { '.', VIS_HELP("Last inserted text")                               },
	[VIS_REG_SEARCH]     = { '/', VIS_HELP("Last search pattern")                              },
	[VIS_REG_COMMAND]    = { ':', VIS_HELP("Last :-command")                                   },
	[VIS_REG_SHELL]      = { '!', VIS_HELP("Last shell command given to either <, >, |, or !") },
	[VIS_REG_NUMBER]     = { '#', VIS_HELP("Register number")                                  },
};
