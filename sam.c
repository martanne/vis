/*
 * Heavily inspired (and partially based upon) Rob Pike's sam text editor
 * for Plan 9. Licensed under the Lucent Public License Version 1.02.
 *
 *  Copyright © 2000-2009 Lucent Technologies
 *  Copyright © 2016 Marc André Tanner <mat at brain-dump.org>
 */
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include "sam.h"
#include "vis-core.h"
#include "buffer.h"
#include "text.h"
#include "text-motions.h"
#include "text-objects.h"
#include "text-regex.h"
#include "util.h"

#define MAX_ARGV 8

typedef struct Address Address;
typedef struct Command Command;
typedef struct CommandDef CommandDef;

typedef struct {        /* used to keep context when dealing with external proceses */
	Vis *vis;       /* editor instance */
	Text *txt;      /* text into which received data will be inserted */
	size_t pos;     /* position at which to insert new data */
} Filter;

struct Address {
	char type;      /* # (char) l (line) g (goto line) / ? . $ + - , ; % */
	Regex *regex;   /* NULL denotes default for x, y, X, and Y commands */
	size_t number;  /* line or character number */
	Address *left;  /* left hand side of a compound address , ; */
	Address *right; /* either right hand side of a compound address or next address */
};

struct Command {
	const char *argv[MAX_ARGV];/* [0]=cmd-name, [1..MAX_ARGV-2]=arguments, last element always NULL */
	Address *address;         /* range of text for command */
	Regex *regex;             /* regex to match, used by x, y, g, v, X, Y */
	const CommandDef *cmddef; /* which command is this? */
	int count;                /* command count if any */
	char flags;               /* command specific flags */
	Command *cmd;             /* target of x, y, g, v, X, Y, { */
	Command *next;            /* next command in {} group */
};

struct CommandDef {
	const char *name[3];                 /* name and optional alias for the command */
	enum {
		CMD_CMD           = 1 << 0,  /* does the command take a sub/target command? */
		CMD_REGEX         = 1 << 1,  /* regex after command? */
		CMD_REGEX_DEFAULT = 1 << 2,  /* is the regex optional i.e. can we use a default? */
		CMD_COUNT         = 1 << 3,  /* does the command support a count as in s2/../? */
		CMD_TEXT          = 1 << 4,  /* does the command need a text to insert? */
		CMD_ADDRESS_NONE  = 1 << 5,  /* is it an error to specify an address for the command? */
		CMD_ADDRESS_POS   = 1 << 6,  /* no address implies an empty range at current cursor position */
		CMD_ADDRESS_LINE  = 1 << 7,  /* if no address is given, use the current line */
		CMD_ADDRESS_AFTER = 1 << 8,  /* if no address is given, begin at the start of the next line */
		CMD_SHELL         = 1 << 9,  /* command needs a shell command as argument */
		CMD_FILE          = 1 << 10, /* does the command take a file name */
		CMD_FORCE         = 1 << 11, /* can the command be forced with ! */
		CMD_ARGV          = 1 << 12, /* whether shell like argument splitted is desired */
		CMD_ONCE          = 1 << 13, /* command should only be executed once, not for every selection */
	} flags;
	const char *defcmd;                  /* name of a default target command */
	bool (*func)(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*); /* command implementation */
};

/* sam commands */
static bool cmd_insert(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_append(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_change(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_delete(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_guard(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_extract(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_select(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_print(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_files(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_pipein(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_pipeout(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_filter(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_launch(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_substitute(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_write(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_read(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_edit(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_quit(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_cd(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
/* vi(m) commands */
static bool cmd_set(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_open(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_bdelete(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_qall(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_split(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_vsplit(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_new(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_vnew(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_wq(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_earlier_later(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_help(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_map(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_unmap(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);
static bool cmd_langmap(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);

/* command recognized at the ':'-prompt. commands are found using a unique
 * prefix match. that is if a command should be available under an abbreviation
 * which is a prefix for another command it has to be added as an alias. the
 * long human readable name should always come first */
static const CommandDef cmds[] = {
	/* name(s),           flags,                    default command, implementation    */
	{ { "a"            }, CMD_TEXT,                            NULL, cmd_append        },
	{ { "c"            }, CMD_TEXT,                            NULL, cmd_change        },
	{ { "d"            }, 0,                                   NULL, cmd_delete        },
	{ { "g"            }, CMD_CMD|CMD_REGEX,                   "p",  cmd_guard         },
	{ { "i"            }, CMD_TEXT,                            NULL, cmd_insert        },
	{ { "p"            }, 0,                                   NULL, cmd_print         },
	{ { "s"            }, CMD_SHELL,                           NULL, cmd_substitute    },
	{ { "v"            }, CMD_CMD|CMD_REGEX,                   "p",  cmd_guard         },
	{ { "x"            }, CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT, "p",  cmd_extract       },
	{ { "y"            }, CMD_CMD|CMD_REGEX,                   "p",  cmd_extract       },
	{ { "X"            }, CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT, NULL, cmd_files         },
	{ { "Y"            }, CMD_CMD|CMD_REGEX,                   NULL, cmd_files         },
	{ { ">"            }, CMD_SHELL|CMD_ADDRESS_LINE,          NULL, cmd_pipeout       },
	{ { "<"            }, CMD_SHELL|CMD_ADDRESS_POS,           NULL, cmd_pipein        },
	{ { "|"            }, CMD_SHELL|CMD_ADDRESS_POS,           NULL, cmd_filter        },
	{ { "!"            }, CMD_SHELL|CMD_ONCE,                  NULL, cmd_launch        },
	{ { "w"            }, CMD_ARGV|CMD_FORCE,                  NULL, cmd_write         },
	{ { "r"            }, CMD_FILE|CMD_ADDRESS_AFTER,          NULL, cmd_read          },
	{ { "{"            }, 0,                                   NULL, NULL              },
	{ { "}"            }, 0,                                   NULL, NULL              },
	{ { "e"            }, CMD_FILE|CMD_FORCE|CMD_ONCE,         NULL, cmd_edit          },
	{ { "q"            }, CMD_FORCE|CMD_ONCE,                  NULL, cmd_quit          },
	{ { "cd"           }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_cd            },
	/* vi(m) related commands */
	{ { "bdelete"      }, CMD_FORCE|CMD_ONCE,                  NULL, cmd_bdelete       },
	{ { "help"         }, CMD_ONCE,                            NULL, cmd_help          },
	{ { "map"          }, CMD_ARGV|CMD_FORCE|CMD_ONCE,         NULL, cmd_map           },
	{ { "map-window"   }, CMD_ARGV|CMD_FORCE|CMD_ONCE,         NULL, cmd_map           },
	{ { "unmap"        }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_unmap         },
	{ { "unmap-window" }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_unmap         },
	{ { "langmap"      }, CMD_ARGV|CMD_FORCE|CMD_ONCE,         NULL, cmd_langmap       },
	{ { "new"          }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_new           },
	{ { "open"         }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_open          },
	{ { "qall"         }, CMD_FORCE|CMD_ONCE,                  NULL, cmd_qall          },
	{ { "set"          }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_set           },
	{ { "split"        }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_split         },
	{ { "vnew"         }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_vnew          },
	{ { "vsplit"       }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_vsplit        },
	{ { "wq"           }, CMD_ARGV|CMD_FORCE|CMD_ONCE,         NULL, cmd_wq            },
	{ { "earlier"      }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_earlier_later },
	{ { "later"        }, CMD_ARGV|CMD_ONCE,                   NULL, cmd_earlier_later },
	{ { NULL           }, 0,                                   NULL, NULL              },
};

static const CommandDef cmddef_select =
	{ { "s"            }, 0,                                   NULL, cmd_select        };

const char *sam_error(enum SamError err) {
	static const char *error_msg[] = {
		[SAM_ERR_OK]              = "Success",
		[SAM_ERR_MEMORY]          = "Out of memory",
		[SAM_ERR_ADDRESS]         = "Bad address",
		[SAM_ERR_NO_ADDRESS]      = "Command takes no address",
		[SAM_ERR_UNMATCHED_BRACE] = "Unmatched `}'",
		[SAM_ERR_REGEX]           = "Bad regular expression",
		[SAM_ERR_TEXT]            = "Bad text",
		[SAM_ERR_FILENAME]        = "Filename expected",
		[SAM_ERR_COMMAND]         = "Unknown command",
		[SAM_ERR_EXECUTE]         = "Error executing command",
	};

	return err < LENGTH(error_msg) ? error_msg[err] : NULL;
}

static Address *address_new(void) {
	return calloc(1, sizeof(Address));
}

static void address_free(Address *addr) {
	if (!addr)
		return;
	text_regex_free(addr->regex);
	address_free(addr->left);
	address_free(addr->right);
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

	if (!delim)
		return NULL;
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
			default:
				if (**s != '\\' && **s != delim)
					buffer_append(&buf, "\\", 1);
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

static char *parse_until(const char **s, const char *until) {
	Buffer buf;
	buffer_init(&buf);
	size_t len = strlen(until);

	while (**s && !memchr(until, **s, len))
		buffer_append(&buf, (*s)++, 1);

	if (buffer_length(&buf))
	    buffer_append(&buf, "\0", 1);

	return buf.data;
}

static char *parse_shellcmd(const char **s) {
	skip_spaces(s);
	return parse_until(s, "\n");
}

static char *parse_filename(const char **s) {
	skip_spaces(s);
	if (**s == '"' || **s == '\'')
		return parse_delimited_text(s);
	return parse_until(s, "\n");
}

static void parse_argv(const char **s, const char *argv[], size_t maxarg) {
	for (size_t i = 0; i < maxarg; i++) {
		skip_spaces(s);
		if (**s == '"' || **s == '\'')
			argv[i] = parse_delimited_text(s);
		else
			argv[i] = parse_until(s, " \t\n");
	}
}

static char *parse_cmdname(const char **s) {
	skip_spaces(s);
	Buffer buf;
	buffer_init(&buf);

	while (isalpha((unsigned char)**s))
		buffer_append(&buf, (*s)++, 1);

	if (buffer_length(&buf))
	    buffer_append(&buf, "\0", 1);

	return buf.data;
}

static Regex *parse_regex(Vis *vis, const char **s) {
	char *pattern = parse_delimited_text(s);
	if (!pattern)
		return NULL;
	Regex *regex = vis_regex(vis, *pattern ? pattern : NULL);
	free(pattern);
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
	case '%':
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
					address_free(addr.right);
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
		address_free(addr.right);
		return NULL;
	}
	*ret = addr;
	return ret;
}

static Address *address_parse_compound(Vis *vis, const char **s, enum SamError *err) {
	Address addr = { 0 }, *left = address_parse_simple(vis, s, err), *right = NULL;
	skip_spaces(s);
	addr.type = **s;
	switch (addr.type) {
	case ',': /* a1,a2 */
	case ';': /* a1;a2 */
		(*s)++;
		right = address_parse_compound(vis, s, err);
		if (right && (right->type == ',' || right->type == ';') && !right->left) {
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
	address_free(left);
	address_free(right);
	return NULL;
}

static Command *command_new(const char *name) {
	Command *cmd = calloc(1, sizeof(Command));
	if (!cmd)
		return NULL;
	if (name && !(cmd->argv[0] = strdup(name))) {
		free(cmd);
		return NULL;
	}
	return cmd;
}

static void command_free(Command *cmd) {
	if (!cmd)
		return;

	for (Command *c = cmd->cmd, *next; c; c = next) {
		next = c->next;
		command_free(c);
	}

	for (const char **args = cmd->argv; *args; args++)
		free((void*)*args);
	address_free(cmd->address);
	text_regex_free(cmd->regex);
	free(cmd);
}

static const CommandDef *command_lookup(Vis *vis, const char *name) {
	if (!vis->cmds) {
		if (!(vis->cmds = map_new()))
			return NULL;

		for (const CommandDef *cmd = cmds; cmd && cmd->name[0]; cmd++) {
			for (const char *const *name = cmd->name; *name; name++)
				map_put(vis->cmds, *name, cmd);
		}
	}
	return map_closest(vis->cmds, name);
}

static Command *command_parse(Vis *vis, const char **s, int level, enum SamError *err) {
	if (!**s)
		return NULL;
	Command *cmd = command_new(NULL);
	if (!cmd)
		return NULL;

	cmd->address = address_parse_compound(vis, s, err);
	skip_spaces(s);

	cmd->argv[0] = parse_cmdname(s);

	if (!cmd->argv[0]) {
		char name[2] = { **s ? **s : 'p', '\0' };
		if (**s)
			(*s)++;
		if (!(cmd->argv[0] = strdup(name)))
			goto fail;
	}

	const CommandDef *cmddef = command_lookup(vis, cmd->argv[0]);
	if (!cmddef) {
		*err = SAM_ERR_COMMAND;
		goto fail;
	}

	cmd->cmddef = cmddef;

	if (strcmp(cmd->argv[0], "{") == 0) {
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
	} else if (strcmp(cmd->argv[0], "}") == 0) {
		if (level == 0) {
			*err = SAM_ERR_UNMATCHED_BRACE;
			goto fail;
		}
		command_free(cmd);
		return NULL;
	}

	if (cmddef->flags & CMD_ADDRESS_NONE && cmd->address) {
		*err = SAM_ERR_NO_ADDRESS;
		goto fail;
	}

	if (cmddef->flags & CMD_COUNT)
		cmd->count = parse_number(s);

	if (cmddef->flags & CMD_FORCE && **s == '!') {
		cmd->flags = '!';
		(*s)++;
	}

	if (cmddef->flags & CMD_FILE) {
		if (!(cmd->argv[1] = parse_filename(s)) && cmd->argv[0][0] != 'w') {
			*err = SAM_ERR_FILENAME;
			goto fail;
		}
	}

	if (cmddef->flags & CMD_REGEX) {
		if ((cmddef->flags & CMD_REGEX_DEFAULT) && (!**s || **s == ' ')) {
			skip_spaces(s);
		} else if (!(cmd->regex = parse_regex(vis, s))) {
			*err = SAM_ERR_REGEX;
			goto fail;
		}
	}

	if (cmddef->flags & CMD_SHELL && !(cmd->argv[1] = parse_shellcmd(s))) {
		*err = SAM_ERR_SHELL;
		goto fail;
	}

	if (cmddef->flags & CMD_TEXT && !(cmd->argv[1] = parse_text(s))) {
		*err = SAM_ERR_TEXT;
		goto fail;
	}

	if (cmddef->flags & CMD_ARGV) {
		parse_argv(s, &cmd->argv[1], MAX_ARGV-2);
		cmd->argv[MAX_ARGV-1] = NULL;
	}

	if (cmddef->flags & CMD_CMD) {
		skip_spaces(s);
		if (cmddef->defcmd && (**s == '\n' || **s == '\0')) {
			if (**s == '\n')
				(*s)++;
			if (!(cmd->cmd = command_new(cmddef->defcmd)))
				goto fail;
			cmd->cmd->cmddef = command_lookup(vis, cmddef->defcmd);
		} else {
			if (!(cmd->cmd = command_parse(vis, s, level, err)))
				goto fail;
			if (strcmp(cmd->argv[0], "X") == 0 || strcmp(cmd->argv[0], "Y") == 0) {
				Command *sel = command_new("s");
				if (!sel)
					goto fail;
				sel->cmd = cmd->cmd;
				sel->cmddef = &cmddef_select;
				cmd->cmd = sel;
			}
		}
	}

	return cmd;
fail:
	command_free(cmd);
	return NULL;
}

static Command *sam_parse(Vis *vis, const char *cmd, enum SamError *err) {
	const char **s = &cmd;
	Command *c = command_parse(vis, s, 0, err);
	if (!c)
		return NULL;
	Command *sel = command_new("s");
	if (!sel) {
		command_free(c);
		return NULL;
	}
	sel->cmd = c;
	sel->cmddef = &cmddef_select;
	return sel;
}

static Filerange address_line_evaluate(Address *addr, File *file, Filerange *range, int sign) {
	Text *txt = file->text;
	size_t offset = addr->number != 0 ? addr->number : 1;
	size_t start = range->start, end = range->end, line;
	if (sign > 0) {
		char c;
		if (end > 0 && text_byte_get(txt, end-1, &c) && c == '\n')
			end--;
		line = text_lineno_by_pos(txt, end);
		line = text_pos_by_lineno(txt, line + offset);
	} else if (sign < 0) {
		line = text_lineno_by_pos(txt, start);
		line = offset < line ? text_pos_by_lineno(txt, line - offset) : 0;
	} else {
		line = text_pos_by_lineno(txt, addr->number);
	}

	if (addr->type == 'g')
		return text_range_new(line, line);
	else
		return text_range_new(line, text_line_next(txt, line));
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
		case 'g':
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
		case '%':
			return text_range_new(0, text_size(file->text));
		}
		if (text_range_valid(&ret))
			range = &ret;
	} while ((addr = addr->right));

	return ret;
}

static bool sam_execute(Vis *vis, Win *win, Command *cmd, Cursor *cur, Filerange *range) {
	bool ret = true;
	if (cmd->address)
		*range = address_evaluate(cmd->address, win->file, range, 0);

	switch (cmd->argv[0][0]) {
	case '{':
	{
		Text *txt = win->file->text;
		Mark start, end;
		Filerange group = *range;

		for (Command *c = cmd->cmd; c && ret; c = c->next) {
			if (!text_range_valid(&group))
				return false;

			start = text_mark_set(txt, group.start);
			end = text_mark_set(txt, group.end);

			ret &= sam_execute(vis, win, c, NULL, &group);

			size_t s = text_mark_get(txt, start);
			/* hack to make delete work, only update if still valid */
			if (s != EPOS)
				group.start = s;
			group.end = text_mark_get(txt, end);
		}
		break;
	}
	default:
		ret = cmd->cmddef->func(vis, win, cmd, cmd->argv, cur, range);
		break;
	}
	return ret;
}

enum SamError sam_cmd(Vis *vis, const char *s) {
	enum SamError err = SAM_ERR_OK;
	if (!s)
		return err;

	Command *cmd = sam_parse(vis, s, &err);
	if (!cmd) {
		if (err == SAM_ERR_OK)
			err = SAM_ERR_MEMORY;
		return err;
	}

	Filerange range = text_range_empty();
	sam_execute(vis, vis->win, cmd, NULL, &range);

	bool completed = true;
	for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
		Filerange sel = view_cursors_selection_get(c);
		if (text_range_valid(&sel)) {
			completed = false;
			break;
		}
	}

	vis_mode_switch(vis, completed ? VIS_MODE_NORMAL : VIS_MODE_VISUAL);
	command_free(cmd);
	return err;
}

static bool cmd_insert(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	size_t len = strlen(argv[1]);
	bool ret = text_insert(win->file->text, range->start, argv[1], len);
	if (ret)
		*range = text_range_new(range->start, range->start + len);
	return ret;
}

static bool cmd_append(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	size_t len = strlen(argv[1]);
	bool ret = text_insert(win->file->text, range->end, argv[1], len);
	if (ret)
		*range = text_range_new(range->end, range->end + len);
	return ret;
}

static bool cmd_change(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Text *txt = win->file->text;
	size_t len = strlen(argv[1]);
	bool ret = text_delete(txt, range->start, text_range_size(range)) &&
	      text_insert(txt, range->start, argv[1], len);
	if (ret)
		*range = text_range_new(range->start, range->start + len);
	return ret;
}

static bool cmd_delete(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool ret = text_delete(win->file->text, range->start, text_range_size(range));
	if (ret)
		*range = text_range_new(range->start, range->start);
	return ret;
}

static bool cmd_guard(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool match = !text_search_range_forward(win->file->text, range->start,
		text_range_size(range), cmd->regex, 0, NULL, 0);
	if (match ^ (argv[0][0] == 'v'))
		return sam_execute(vis, win, cmd->cmd, cur, range);
	view_cursors_dispose(cur);
	return true;
}

static bool cmd_extract(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool ret = true;
	View *view = win->view;
	Text *txt = win->file->text;

	Cursor *cursor = NULL;
	size_t pos = view_cursor_get(view);
	if (cur && !view_cursors_dispose(cur)) {
		cursor = view_cursors_new(view, text_size(txt)+1);
		view_cursors_dispose(cur);
	}

	if (cmd->regex) {
		size_t start = range->start, end = range->end, last_start = EPOS;
		RegexMatch match[1];
		while (start < end) {
			bool found = text_search_range_forward(txt, start,
				end - start, cmd->regex, 1, match, 0) == 0;
			Filerange r = text_range_empty();
			if (found) {
				if (argv[0][0] == 'x')
					r = text_range_new(match[0].start, match[0].end);
				else
					r = text_range_new(start, match[0].start);
				if (match[0].start == match[0].end) {
					if (last_start == match[0].start) {
						start++;
						continue;
					}
					start = match[0].end+1;
				} else {
					start = match[0].end;
				}
			} else {
				if (argv[0][0] == 'y')
					r = text_range_new(start, end);
				start = end;
			}

			if (text_range_valid(&r)) {
				Mark mark_start = text_mark_set(txt, start);
				Mark mark_end = text_mark_set(txt, end);
				ret &= sam_execute(vis, win, cmd->cmd, NULL, &r);
				last_start = start = text_mark_get(txt, mark_start);
				if (start == EPOS && last_start != r.end)
					last_start = start = r.end;
				end = text_mark_get(txt, mark_end);
				if (start == EPOS || end == EPOS) {
					ret = false;
					break;
				}
			}
		}
	} else {
		size_t start = range->start, end = range->end;
		while (start < end) {
			size_t next = text_line_next(txt, start);
			if (next > end)
				next = end;
			Filerange r = text_range_new(start, next);
			if (start == next || !text_range_valid(&r))
				break;
			start = next;
			Mark mark_start = text_mark_set(txt, start);
			Mark mark_end = text_mark_set(txt, end);
			ret &= sam_execute(vis, win, cmd->cmd, NULL, &r);
			start = text_mark_get(txt, mark_start);
			if (start == EPOS)
				start = r.end;
			end = text_mark_get(txt, mark_end);
			if (end == EPOS) {
				ret = false;
				break;
			}
		}
	}

	if (cursor && !view_cursors_dispose(cursor)) {
		view_cursors_selection_clear(cursor);
		view_cursors_to(cursor, pos);
	}

	return ret;
}

static bool cmd_select(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool ret = true;
	View *view = win->view;
	Text *txt = win->file->text;
	bool multiple_cursors = view_cursors_multiple(view);
	Cursor *primary = view_cursors_primary_get(view);

	for (Cursor *c = view_cursors(view), *next; c && ret; c = next) {
		next = view_cursors_next(c);
		Filerange sel;
		size_t pos = view_cursors_pos(c);
		if (vis->mode->visual) {
			sel = view_cursors_selection_get(c);
		} else if (cmd->cmd->address) {
			/* convert a single line range to a goto line motion */
			if (!multiple_cursors && cmd->cmd->cmddef->func == cmd_print) {
				Address *addr = cmd->cmd->address;
				switch (addr->type) {
				case '+':
				case '-':
					addr = addr->right;
					/* fall through */
				case 'l':
					if (addr && addr->type == 'l' && !addr->right)
						addr->type = 'g';
					break;
				}
			}
			sel = text_range_new(pos, pos);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_POS) {
			sel = text_range_new(pos, pos);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_LINE) {
			sel = text_object_line(txt, pos);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_AFTER) {
			size_t next_line = text_line_next(txt, pos);
			sel = text_range_new(next_line, next_line);
		} else if (multiple_cursors) {
			sel = text_object_line(txt, pos);
		} else {
			sel = text_range_new(0, text_size(txt));
		}
		if (text_range_valid(&sel))
			ret &= sam_execute(vis, win, cmd->cmd, c, &sel);
		if (cmd->cmd->cmddef->flags & CMD_ONCE)
			break;
	}

	if (view == vis->win->view && primary != view_cursors_primary_get(view))
		view_cursors_primary_set(view_cursors(view));
	return ret;
}

static bool cmd_print(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!text_range_valid(range))
		return false;
	View *view = win->view;
	Text *txt = win->file->text;
	size_t pos = range->end;
	if (range->start != range->end)
		pos = text_char_prev(txt, pos);
	if (cur)
		view_cursors_to(cur, pos);
	else
		cur = view_cursors_new(view, pos);
	if (cur) {
		if (range->start != range->end)
			view_cursors_selection_set(cur, range);
		else
			view_cursors_selection_clear(cur);
	}
	return cur != NULL;
}

static bool cmd_files(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	bool ret = true;
	for (Win *win = vis->windows; win; win = win->next) {
		if (win->file->internal)
			continue;
		bool match = !cmd->regex || (win->file->name &&
		             text_regex_match(cmd->regex, win->file->name, 0));
		if (match ^ (argv[0][0] == 'Y'))
			ret &= sam_execute(vis, win, cmd->cmd, NULL, range);
	}
	return ret;
}

static bool cmd_substitute(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Buffer buf;
	buffer_init(&buf);
	bool ret = false;
	if (buffer_put0(&buf, "s") && buffer_append0(&buf, argv[1]))
		ret = cmd_filter(vis, win, cmd, (const char*[]){ argv[0], "sed", buf.data, NULL }, cur, range);
	buffer_release(&buf);
	return ret;
}

static bool cmd_write(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	File *file = win->file;
	Text *text = file->text;
	if (!argv[1])
		argv[1] = file->name ? strdup(file->name) : NULL;
	if (!argv[1]) {
		if (file->is_stdin) {
			if (strchr(argv[0], 'q')) {
				ssize_t written = text_write_range(text, range, STDOUT_FILENO);
				if (written == -1 || (size_t)written != text_range_size(range)) {
					vis_info_show(vis, "Can not write to stdout");
					return false;
				}
				/* make sure the file is marked as saved i.e. not modified */
				text_save_range(text, range, NULL);
				return true;
			}
			vis_info_show(vis, "No filename given, use 'wq' to write to stdout");
			return false;
		}
		vis_info_show(vis, "Filename expected");
		return false;
	}

	for (const char **name = &argv[1]; *name; name++) {
		struct stat meta;
		if (cmd->flags != '!' && file->stat.st_mtime && stat(*name, &meta) == 0 &&
		    file->stat.st_mtime < meta.st_mtime) {
			vis_info_show(vis, "WARNING: file has been changed since reading it");
			return false;
		}
		if (!text_save_range(text, range, *name)) {
			vis_info_show(vis, "Can't write `%s'", *name);
			return false;
		}
		if (!file->name) {
			vis_window_name(win, *name);
			file->name = win->file->name;
		}
		if (strcmp(file->name, *name) == 0)
			file->stat = text_stat(text);
		if (vis->event && vis->event->file_save)
			vis->event->file_save(vis, file);
	}
	return true;
}

static bool cmd_read(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Buffer buf;
	buffer_init(&buf);
	bool ret = false;
	if (buffer_put0(&buf, "cat ") && buffer_append0(&buf, argv[1]))
		ret = cmd_pipein(vis, win, cmd, (const char*[]){ argv[0], buf.data, NULL }, cur, range);
	buffer_release(&buf);
	return ret;
}

static ssize_t read_text(void *context, char *data, size_t len) {
	Filter *filter = context;
	text_insert(filter->txt, filter->pos, data, len);
	filter->pos += len;
	return len;
}

static ssize_t read_buffer(void *context, char *data, size_t len) {
	buffer_append(context, data, len);
	return len;
}

static bool cmd_filter(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Text *txt = win->file->text;

	/* The general idea is the following:
	 *
	 *  1) take a snapshot
	 *  2) write [range.start, range.end] to external command
	 *  3) read the output of the external command and insert it after the range
	 *  4) depending on the exit status of the external command
	 *     - on success: delete original range
	 *     - on failure: revert to previous snapshot
	 *
	 *  2) and 3) happen in small chunks
	 */

	text_snapshot(txt);

	Filter filter = {
		.vis = vis,
		.txt = txt,
		.pos = range->end,
	};

	Buffer buferr;
	buffer_init(&buferr);

	int status = vis_pipe(vis, range, &argv[1], &filter, read_text, &buferr, read_buffer);

	if (status == 0) {
		text_delete_range(txt, range);
		text_snapshot(txt);
		range->end = filter.pos - text_range_size(range);
		if (cur)
			view_cursors_to(cur, range->start);
	} else {
		/* make sure we have somehting to undo */
		text_insert(txt, filter.pos, " ", 1);
		text_undo(txt);
	}

	if (vis->cancel_filter)
		vis_info_show(vis, "Command cancelled");
	else if (status == 0)
		; //vis_info_show(vis, "Command succeded");
	else
		vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));

	buffer_release(&buferr);

	return !vis->cancel_filter && status == 0;
}

static bool cmd_launch(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Filerange empty = text_range_new(range->start, EPOS);
	return cmd_filter(vis, win, cmd, argv, cur, &empty);
}

static bool cmd_pipein(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Filerange filter_range = text_range_new(range->end, range->end);
	bool ret = cmd_filter(vis, win, cmd, (const char*[]){ argv[0], argv[1], NULL }, cur, &filter_range);
	if (ret) {
		text_delete_range(win->file->text, range);
		range->end = range->start + text_range_size(&filter_range);
		if (cur)
			view_cursors_to(cur, range->start);
	}
	return ret;
}

static bool cmd_pipeout(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Buffer buferr;
	buffer_init(&buferr);

	int status = vis_pipe(vis, range, (const char*[]){ argv[1], NULL }, NULL, NULL, &buferr, read_buffer);

	if (status == 0 && cur)
		view_cursors_to(cur, range->start);

	if (vis->cancel_filter)
		vis_info_show(vis, "Command cancelled");
	else if (status == 0)
		; //vis_info_show(vis, "Command succeded");
	else
		vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));

	buffer_release(&buferr);

	return !vis->cancel_filter && status == 0;
}

static bool cmd_cd(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	const char *dir = argv[1];
	if (!dir)
		dir = getenv("HOME");
	return dir && chdir(dir) == 0;
}

#include "vis-cmds.c"
