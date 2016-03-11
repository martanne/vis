/*
 * Heavily inspired (and partially based upon) Rob Pike's sam text editor
 * for Plan 9. Licensed under the Lucent Public License Version 1.02.
 *
 *  Copyright © 2000-2009 Lucent Technologies
 *  Copyright © 2016 Marc André Tanner <mat at brain-dump.org>
 */
#include <string.h>
#include <stdio.h>
#include "sam.h"
#include "vis-core.h"
#include "buffer.h"
#include "text.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-regex.h"
#include "util.h"

typedef struct Address Address;
typedef struct Command Command;
typedef struct CommandDef CommandDef;

struct Address {
	char type;      /* # (char) l (line) / ? . $ + - , ; * */
	Regex *regex;   /* NULL denotes default for x, y, X, and Y commands */
	size_t number;  /* line or character number */
	Address *left;  /* left hand side of a compound address , ; */
	Address *right; /* either right hand side of a compound address or next address */
};

struct Command {
	char name;                /* "index" into command table */
	Address *address;         /* range of text for command */
	Regex *regex;             /* regex to match, used by x, y, g, v, X, Y */
	char *text;               /* text to insert, used by i, a, c */
	const CommandDef *cmddef; /* which command is this? */
	int count;                /* command count if any */
	char flag;                /* command specific flags */
	Command *cmd;             /* target of x, y, g, v, X, Y, { */
	Command *next;            /* next command in {} group */
};

struct CommandDef {
	char name;
	enum {
		CMD_CMD           = 1 << 0, /* does the command take a sub/target command? */
		CMD_REGEX         = 1 << 1, /* regex after command? */
		CMD_REGEX_DEFAULT = 1 << 2, /* is the regex optional i.e. can we use a default? */
		CMD_COUNT         = 1 << 3, /* does the command support a count as in s2/../? */
		CMD_TEXT          = 1 << 4, /* does the command need a text to insert? */
		CMD_ADDRESS_NONE  = 1 << 5, /* is it an error to specify an address for the command */
		CMD_ADDRESS_ALL   = 1 << 6, /* if no address is given, use the whole file */
		CMD_SHELL         = 1 << 7, /* command needs a shell command as argument */
	} flags;
	char defcmd;                        /* name of a default target command */
	bool (*func)(Vis*, Win*, Command*, Filerange*); /* command imiplementation */
};

static bool cmd_insert(Vis*, Win*, Command*, Filerange*);
static bool cmd_append(Vis*, Win*, Command*, Filerange*);
static bool cmd_change(Vis*, Win*, Command*, Filerange*);
static bool cmd_delete(Vis*, Win*, Command*, Filerange*);
static bool cmd_guard(Vis*, Win*, Command*, Filerange*);
static bool cmd_extract(Vis*, Win*, Command*, Filerange*);
static bool cmd_select(Vis*, Win*, Command*, Filerange*);
static bool cmd_print(Vis*, Win*, Command*, Filerange*);
static bool cmd_files(Vis*, Win*, Command*, Filerange*);
static bool cmd_shell(Vis*, Win*, Command*, Filerange*);
static bool cmd_substitute(Vis*, Win*, Command*, Filerange*);

static const CommandDef cmds[] = {
	/* name, flags,                default command,  command        */
	{ 'a', CMD_TEXT,                             0,  cmd_append     },
	{ 'c', CMD_TEXT,                             0,  cmd_change     },
	{ 'd', 0,                                    0,  cmd_delete     },
	{ 'g', CMD_CMD|CMD_REGEX,                   'p', cmd_guard      },
	{ 'i', CMD_TEXT,                             0,  cmd_insert     },
	{ 'p', 0,                                    0,  cmd_print      },
	{ 's', CMD_TEXT,                             0,  cmd_substitute },
	{ 'v', CMD_CMD|CMD_REGEX,                   'p', cmd_guard      },
	{ 'x', CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT, 'p', cmd_extract    },
	{ 'y', CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT, 'p', cmd_extract    },
	{ 'X', CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT,  0,  cmd_files      },
	{ 'Y', CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT,  0,  cmd_files      },
	{ '!', CMD_SHELL|CMD_ADDRESS_NONE,           0,  cmd_shell      },
	{ '>', CMD_SHELL,                            0,  cmd_shell      },
	{ '<', CMD_SHELL,                            0,  cmd_shell      },
	{ '|', CMD_SHELL,                            0,  cmd_shell      },
	{ 0 /* array terminator */                                      },
};

static const CommandDef cmds_internal[] = {
	{ 's', 0,                                    0,  cmd_select     },
	{ 0 /* array terminator */                                      },
};

const char *sam_error(enum SamError err) {
	static const char *error_msg[] = {
		[SAM_ERR_OK]              = "Success",
		[SAM_ERR_MEMORY]          = "Out of memory",
		[SAM_ERR_ADDRESS]         = "Bad address",
		[SAM_ERR_NO_ADDRESS]      = "Command takes no address",
		[SAM_ERR_UNMATCHED_BRACE] = "Unmatched `}'",
		[SAM_ERR_REGEX]           = "Bad regular expression",
		[SAM_ERR_TEXT]            = "Bad text",
		[SAM_ERR_COMMAND]         = "Unknown command",
		[SAM_ERR_EXECUTE]         = "Error executing command",
	};

	return err < LENGTH(error_msg) ? error_msg[err] : NULL;
}

static Address *address_new(void) {
	return calloc(1, sizeof(Address));
}

static void address_free(Vis *vis, Address *addr) {
	if (!addr)
		return;
	if (addr->regex != vis->search_pattern)
		text_regex_free(addr->regex);
	address_free(vis, addr->left);
	address_free(vis, addr->right);
	free(addr);
}

static void skip_spaces(const char **s) {
	while (**s == ' ' || **s == '\t')
		(*s)++;
}

static char *parse_delimited_text(const char **s) {
	Buffer buf;
	bool escaped = false;
	char delim = **s;

	buffer_init(&buf);

	for ((*s)++; **s && (**s != delim || escaped); (*s)++) {
		if (!escaped && **s == '\\') {
			escaped = true;
			continue;
		}

		char c = **s;

		if (escaped) {
			escaped = false;
			switch (**s) {
			case '\n':
				continue;
			case 'n':
				c = '\n';
				break;
			case 't':
				c = '\t';
				break;
			}
		}

		if (!buffer_append(&buf, &c, 1)) {
			buffer_release(&buf);
			return NULL;
		}
	}

	if (**s == delim)
		(*s)++;

	if (!buffer_append(&buf, "\0", 1)) {
		buffer_release(&buf);
		return NULL;
	}

	return buf.data;
}

static char *parse_text(const char **s) {
	skip_spaces(s);
	if (**s != '\n')
		return parse_delimited_text(s);

	Buffer buf;
	buffer_init(&buf);
	const char *start = *s + 1;
	bool dot = false;

	for ((*s)++; **s && (!dot || **s != '\n'); (*s)++)
		dot = (**s == '.');

	if (!dot || !buffer_put(&buf, start, *s - start - 1) ||
	    !buffer_append(&buf, "\0", 1)) {
		buffer_release(&buf);
		return NULL;
	}

	return buf.data;
}

static Regex *parse_regex(Vis *vis, const char **s) {
	Buffer buf;
	bool escaped = false;
	char delim = **s;
	buffer_init(&buf);

	for ((*s)++; **s && (**s != delim || escaped); (*s)++) {
		if (!escaped && **s == '\\') {
			escaped = true;
			continue;
		}
		if (escaped) {
			escaped = false;
			if (**s != delim)
				buffer_append(&buf, "\\", 1);
		}
		if (!buffer_append(&buf, *s, 1)) {
			buffer_release(&buf);
			return NULL;
		}
	}

	buffer_append(&buf, "\0", 1);

	Regex *regex = NULL;

	if (**s == delim || **s == '\0') {
		if (**s == delim)
			(*s)++;
		if (buffer_length0(&buf) == 0) {
			regex = vis->search_pattern;
		} else if ((regex = text_regex_new())) {
			if (text_regex_compile(regex, buf.data, REG_EXTENDED|REG_NEWLINE) != 0) {
				text_regex_free(regex);
				regex = NULL;
			} else {
				text_regex_free(vis->search_pattern);
				vis->search_pattern = regex;
			}
		}
	}

	buffer_release(&buf);
	return regex;
}

static int parse_number(const char **s) {
	char *end = NULL;
	int number = strtoull(*s, &end, 10);
	if (end == *s)
		return 1;
	*s = end;
	return number;
}

static Address *address_parse_simple(Vis *vis, const char **s, enum SamError *err) {

	skip_spaces(s);

	Address addr = {
		.type = **s,
		.regex = NULL,
		.number = 0,
		.left = NULL,
		.right = NULL,
	};

	switch (addr.type) {
	case '#': /* character #n */
		(*s)++;
		addr.number = parse_number(s);
		break;
	case '0': case '1': case '2': case '3': case '4': /* line n */
	case '5': case '6': case '7': case '8': case '9':
		addr.type = 'l';
		addr.number = parse_number(s);
		break;
	case '/': /* regexp forwards */
	case '?': /* regexp backwards */
		addr.regex = parse_regex(vis, s);
		if (!addr.regex) {
			*err = SAM_ERR_REGEX;
			return NULL;
		}
		break;
	case '$': /* end of file */
	case '.':
	case '+':
	case '-':
		(*s)++;
		break;
	default:
		return NULL;
	}

	if ((addr.right = address_parse_simple(vis, s, err))) {
		switch (addr.right->type) {
		case '.':
		case '$':
			return NULL;
		case '#':
		case 'l':
		case '/':
		case '?':
			if (addr.type != '+' && addr.type != '-') {
				Address *plus = address_new();
				if (!plus) {
					address_free(vis, addr.right);
					return NULL;
				}
				plus->type = '+';
				plus->right = addr.right;
				addr.right = plus;
			}
			break;
		}
	}

	Address *ret = address_new();
	if (!ret) {
		address_free(vis, addr.right);
		return NULL;
	}
	*ret = addr;
	return ret;
}

static Address *address_parse_compound(Vis *vis, const char **s, enum SamError *err) {
	Address addr = { 0 }, *left = address_parse_simple(vis, s, err), *right = NULL;
	if (!left)
		return NULL;
	skip_spaces(s);
	addr.type = **s;
	switch (addr.type) {
	case ',': /* a1,a2 */
	case ';': /* a1;a2 */
		(*s)++;
		right = address_parse_compound(vis, s, err);
		if (!right || ((right->type == ',' || right->type == ';') && !right->left)) {
			if (right)
				*err = SAM_ERR_ADDRESS;
			goto fail;
		}
		break;
	default:
		return left;
	}

	addr.left = left;
	addr.right = right;

	Address *ret = address_new();
	if (ret) {
		*ret = addr;
		return ret;
	}

fail:
	address_free(vis, left);
	address_free(vis, right);
	return NULL;
}

static Command *command_new(void) {
	return calloc(1, sizeof(Command));
}

static void command_free(Vis *vis, Command *cmd) {
	if (!cmd)
		return;

	for (Command *c = cmd->cmd, *next; c; c = next) {
		next = c->next;
		command_free(vis, c);
	}

	address_free(vis, cmd->address);
	if (cmd->regex != vis->search_pattern)
		text_regex_free(cmd->regex);
	free(cmd->text);
	free(cmd);
}

static const CommandDef *command_lookup(const CommandDef *cmds, char name) {
	if (!name)
		name = 'p';
	for (const CommandDef *cmd = cmds; cmd->name; cmd++) {
		if (cmd->name == name)
			return cmd;
	}
	return NULL;
}

static Command *command_parse(Vis *vis, const char **s, int level, enum SamError *err) {
	Command *cmd = command_new();
	if (!cmd)
		return NULL;

	cmd->address = address_parse_compound(vis, s, err);
	skip_spaces(s);

	cmd->name = **s;

	const CommandDef *cmddef = command_lookup(cmds, cmd->name);

	if (!cmddef) {
		/* let it point to an all zero dummy entry */
		cmddef = &cmds_internal[LENGTH(cmds_internal)-1];
		switch (cmd->name) {
		case '{':
		{
			(*s)++;
			Command *prev = NULL, *next;
			do {
				skip_spaces(s);
				if (**s == '\n')
					(*s)++;
				next = command_parse(vis, s, level+1, err);
				if (prev)
					prev->next = next;
				else
					cmd->cmd = next;
			} while ((prev = next));
			break;
		}
		case '}':
			if (level == 0) {
				*err = SAM_ERR_UNMATCHED_BRACE;
				goto fail;
			}
			(*s)++;
			command_free(vis, cmd);
			return NULL;
		default:
			*err = SAM_ERR_COMMAND;
			goto fail;
		}
	}

	cmd->cmddef = cmddef;

	(*s)++; /* skip command name */

	if (cmddef->flags & CMD_ADDRESS_NONE && cmd->address) {
		*err = SAM_ERR_NO_ADDRESS;
		goto fail;
	}

	if (cmddef->flags & CMD_COUNT)
		cmd->count = parse_number(s);

	if (cmddef->flags & CMD_REGEX) {
		if (cmddef->flags & CMD_REGEX_DEFAULT && **s == ' ') {
			skip_spaces(s);
		} else if (!(cmd->regex = parse_regex(vis, s))) {
			*err = SAM_ERR_REGEX;
			goto fail;
		}
	}

	if (cmddef->flags & CMD_TEXT && !(cmd->text = parse_text(s))) {
		*err = SAM_ERR_TEXT;
		goto fail;
	}

	if (cmddef->flags & CMD_CMD) {
		skip_spaces(s);
		if (cmddef->defcmd && (**s == '\n' || **s == '\0')) {
			if (**s == '\n')
				(*s)++;
			if (!(cmd->cmd = command_new()))
				goto fail;
			cmd->cmd->name = cmddef->defcmd;
			cmd->cmd->cmddef = command_lookup(cmds, cmddef->defcmd);
		} else {
			if (!(cmd->cmd = command_parse(vis, s, level, err)))
				goto fail;
			if (cmd->name == 'X' || cmd->name == 'Y') {
				Command *sel = command_new();
				if (!sel)
					goto fail;
				sel->cmd = cmd->cmd;
				sel->cmddef = command_lookup(cmds_internal, 's');
				cmd->cmd = sel;
			}
		}
	}

	if (!cmd->address) {
		if (cmddef->flags & CMD_ADDRESS_ALL) {
			if (!(cmd->address = address_new()))
				goto fail;
			cmd->address->type = '*';
		}
	}

	return cmd;
fail:
	command_free(vis, cmd);
	return NULL;
}

static Command *sam_parse(Vis *vis, const char *cmd, enum SamError *err) {
	const char **s = &cmd;
	Command *c = command_parse(vis, s, 0, err);
	if (!c)
		return NULL;
	Command *sel = command_new();
	if (!sel) {
		command_free(vis, c);
		return NULL;
	}
	sel->cmd = c;
	sel->cmddef = command_lookup(cmds_internal, 's');
	return sel;
}

static Filerange address_line_evaluate(Address *addr, File *file, Filerange *range, int sign) {
	size_t offset = addr->number != 0 ? addr->number : 1;
	size_t line;
	if (sign > 0) {
		line = text_lineno_by_pos(file->text, range->end);
		line = text_pos_by_lineno(file->text, line + offset);
	} else if (sign < 0) {
		line = text_lineno_by_pos(file->text, range->start);
		line = offset < line ? text_pos_by_lineno(file->text, line - offset) : 0;
	} else {
		line = text_pos_by_lineno(file->text, addr->number);
	}
	return text_range_new(line, text_line_next(file->text, line));
}

static Filerange address_evaluate(Address *addr, File *file, Filerange *range, int sign) {
	Filerange ret = text_range_empty();

	do {
		switch (addr->type) {
		case '#':
			if (sign > 0)
				ret.start = ret.end = range->end + addr->number;
			else if (sign < 0)
				ret.start = ret.end = range->start - addr->number;
			else
				ret = text_range_new(addr->number, addr->number);
			break;
		case 'l':
			ret = address_line_evaluate(addr, file, range, sign);
			break;
		case '?':
			sign = sign == 0 ? -1 : -sign;
			/* fall through */
		case '/':
			if (sign >= 0)
				ret = text_object_search_forward(file->text, range->end, addr->regex);
			else
				ret = text_object_search_backward(file->text, range->start, addr->regex);
			break;
		case '$':
		{
			size_t size = text_size(file->text);
			ret = text_range_new(size, size);
			break;
		}
		case '.':
			ret = *range;
			break;
		case '+':
		case '-':
			sign = addr->type == '+' ? +1 : -1;
			if (!addr->right || addr->right->type == '+' || addr->right->type == '-')
				ret = address_line_evaluate(addr, file, range, sign);
			break;
		case ',':
		case ';':
		{
			Filerange left, right;
			if (addr->left)
				left = address_evaluate(addr->left, file, range, 0);
			else
				left = text_range_new(0, 0);

			if (addr->type == ';')
				range = &left;

			if (addr->right) {
				right = address_evaluate(addr->right, file, range, 0);
			} else {
				size_t size = text_size(file->text);
				right = text_range_new(size, size);
			}
			/* TODO: enforce strict ordering? */
			return text_range_union(&left, &right);
		}
		case '*':
			return text_range_new(0, text_size(file->text));
		}
		if (text_range_valid(&ret))
			range = &ret;
	} while ((addr = addr->right));

	return ret;
}

static bool sam_execute(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	bool ret = true;
	Filerange r = cmd->address ? address_evaluate(cmd->address, win->file, range, 0) : *range;

	switch (cmd->name) {
	case '{':
	{
		Text *txt = win->file->text;
		Mark start, end;
		Filerange group = r;

		for (Command *c = cmd->cmd; c; c = c->next) {
			if (!text_range_valid(&group))
				return false;

			start = text_mark_set(txt, group.start);
			end = text_mark_set(txt, group.end);

			ret &= sam_execute(vis, win, c, &group);

			size_t s = text_mark_get(txt, start);
			/* hack to make delete work, only update if still valid */
			if (s != EPOS)
				group.start = s;
			group.end = text_mark_get(txt, end);
		}
		break;
	}
	default:
		ret = cmd->cmddef->func(vis, win, cmd, &r);
		break;
	}
	return ret;
}

enum SamError sam_cmd(Vis *vis, const char *s) {
	enum SamError err = SAM_ERR_OK;
	Command *cmd = sam_parse(vis, s, &err);
	if (!cmd) {
		if (err == SAM_ERR_OK)
			err = SAM_ERR_MEMORY;
		return err;
	}
	Filerange range = text_range_empty();
	bool status = sam_execute(vis, vis->win, cmd, &range);
	vis_mode_switch(vis, status ? VIS_MODE_NORMAL : VIS_MODE_VISUAL);
	command_free(vis, cmd);
	return err;
}

static bool cmd_insert(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	size_t len = strlen(cmd->text);
	bool ret = text_insert(win->file->text, range->start, cmd->text, len);
	if (ret)
		*range = text_range_new(range->start, range->start + len);
	return ret;
}

static bool cmd_append(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	size_t len = strlen(cmd->text);
	bool ret = text_insert(win->file->text, range->end, cmd->text, len);
	if (ret)
		*range = text_range_new(range->end, range->end + len);
	return ret;
}

static bool cmd_change(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	Text *txt = win->file->text;
	size_t len = strlen(cmd->text);
	bool ret = text_delete(txt, range->start, text_range_size(range)) &&
	      text_insert(txt, range->start, cmd->text, len);
	if (ret)
		*range = text_range_new(range->start, range->start + len);
	return ret;
}

static bool cmd_delete(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	bool ret = text_delete(win->file->text, range->start, text_range_size(range));
	if (ret)
		*range = text_range_new(range->start, range->start);
	return ret;
}

static bool cmd_guard(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	bool match = !text_search_range_forward(win->file->text, range->start,
		text_range_size(range), cmd->regex, 0, NULL, 0);
	if (match ^ (cmd->name == 'v'))
		return sam_execute(vis, win, cmd->cmd, range);
	return true;
}

static bool cmd_extract(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	bool ret = true;
	Text *txt = win->file->text;
	if (cmd->regex) {
		size_t start = range->start, end = range->end;
		RegexMatch match[1];
		while (start < end) {
			bool found = text_search_range_forward(txt, start,
				end - start, cmd->regex, 1, match, 0) == 0 &&
				match[0].start != match[0].end;
			Filerange r = text_range_empty();
			if (found) {
				if (cmd->name == 'x')
					r = text_range_new(match[0].start, match[0].end);
				else
					r = text_range_new(start, match[0].start);
				start = match[0].end;
			} else {
				if (cmd->name == 'y')
					r = text_range_new(start, end);
				start = end;
			}

			if (text_range_valid(&r) && r.start != r.end) {
				Mark mark_start = text_mark_set(txt, start);
				Mark mark_end = text_mark_set(txt, end);
				ret &= sam_execute(vis, win, cmd->cmd, &r);
				start = text_mark_get(txt, mark_start);
				end = text_mark_get(txt, mark_end);
				if (start == EPOS || end == EPOS)
					return false;
			}
		}
	} else {
		size_t start = range->start;
		for (;;) {
			size_t end = text_line_next(txt, start);
			Filerange line = text_range_new(start, end);
			if (start == end || !text_range_valid(&line))
				break;
			Mark mark_end = text_mark_set(txt, end);
			ret &= sam_execute(vis, win, cmd->cmd, &line);
			start = text_mark_get(txt, mark_end);
			if (start == EPOS)
				return false;
		}
	}
	return ret;
}

static bool cmd_select(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	bool ret = true;
	View *view = win->view;
	Text *txt = win->file->text;
	bool multiple_cursors = view_cursors_count(view) > 1;
	for (Cursor *c = view_cursors(view), *next; c; c = next) {
		next = view_cursors_next(c);
		Filerange sel;
		if (vis->mode->visual) {
			sel = view_cursors_selection_get(c);
		} else if (cmd->cmd->address) {
			size_t start = view_cursors_pos(c);
			size_t end = text_char_next(txt, start);
			sel = text_range_new(start, end);
		} else if (multiple_cursors) {
			sel = text_object_line(txt, view_cursors_pos(c));
		} else {
			sel = text_range_new(0, text_size(txt));
		}
		ret &= sam_execute(vis, win, cmd->cmd, &sel);
		view_cursors_dispose(c);
	}
	return ret;
}

static bool cmd_print(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	View *view = win->view;
	Text *txt = win->file->text;
	Cursor *cursor = view_cursors_new(view);
	if (cursor) {
		view_cursors_selection_set(cursor, range);
		view_cursors_to(cursor, text_char_prev(txt, range->end));
	}
	/* indicate "failure"/incomplete command to keep visual mode */
	return false;
}

static bool cmd_files(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	bool ret = true;
	for (Win *win = vis->windows; win; win = win->next) {
		if (win->file->internal)
			continue;
		bool match = !cmd->regex || (win->file->name &&
		             text_regex_match(cmd->regex, win->file->name, 0));
		if (match ^ (cmd->name == 'Y'))
			ret &= sam_execute(vis, win, cmd->cmd, range);
	}
	return ret;
}

static bool cmd_shell(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	return false;
}

static bool cmd_substitute(Vis *vis, Win *win, Command *cmd, Filerange *range) {
	return false;
}
