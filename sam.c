/*
 * Heavily inspired (and partially based upon) the X11 version of
 * Rob Pike's sam text editor originally written for Plan 9.
 *
 *  Copyright © 2016 Marc André Tanner <mat at brain-dump.org>
 *  Copyright © 1998 by Lucent Technologies
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY. IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
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
	const char *name;                    /* command name */
	const char *help;                    /* short, one-line help text */
	enum {
		CMD_NONE          = 0,       /* standalone command without any arguments */
		CMD_CMD           = 1 << 0,  /* does the command take a sub/target command? */
		CMD_REGEX         = 1 << 1,  /* regex after command? */
		CMD_REGEX_DEFAULT = 1 << 2,  /* is the regex optional i.e. can we use a default? */
		CMD_COUNT         = 1 << 3,  /* does the command support a count as in s2/../? */
		CMD_TEXT          = 1 << 4,  /* does the command need a text to insert? */
		CMD_ADDRESS_NONE  = 1 << 5,  /* is it an error to specify an address for the command? */
		CMD_ADDRESS_POS   = 1 << 6,  /* no address implies an empty range at current cursor position */
		CMD_ADDRESS_LINE  = 1 << 7,  /* if no address is given, use the current line */
		CMD_ADDRESS_AFTER = 1 << 8,  /* if no address is given, begin at the start of the next line */
		CMD_ADDRESS_ALL   = 1 << 9,  /* if no address is given, apply to whole file (independent of #cursors) */
		CMD_ADDRESS_ALL_1CURSOR = 1 << 10, /* if no address is given and only 1 cursor exists, apply to whole file */
		CMD_SHELL         = 1 << 11, /* command needs a shell command as argument */
		CMD_FORCE         = 1 << 12, /* can the command be forced with ! */
		CMD_ARGV          = 1 << 13, /* whether shell like argument splitting is desired */
		CMD_ONCE          = 1 << 14, /* command should only be executed once, not for every selection */
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
static bool cmd_user(Vis*, Win*, Command*, const char *argv[], Cursor*, Filerange*);

static const CommandDef cmds[] = {
	//      name            help
	//      flags, default command, implemenation
	{
		"a",            "Append text after range",
		CMD_TEXT, NULL, cmd_append
	}, {
		"c",            "Change text in range",
		CMD_TEXT, NULL, cmd_change
	}, {
		"d",            "Delete text in range",
		CMD_NONE, NULL, cmd_delete
	}, {
		"g",            "If range contains regexp, run command",
		CMD_CMD|CMD_REGEX, "p", cmd_guard
	}, {
		"i",            "Insert text before range",
		CMD_TEXT, NULL, cmd_insert
	}, {
		"p",            "Create selection covering range",
		CMD_NONE, NULL, cmd_print
	}, {
		"s",            "Substitute text for regexp in range",
		CMD_SHELL|CMD_ADDRESS_LINE, NULL, cmd_substitute
	}, {
		"v",            "If range does not contain regexp, run command",
		CMD_CMD|CMD_REGEX, "p", cmd_guard
	}, {
		"x",            "Set range and run command on each match",
		CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT|CMD_ADDRESS_ALL_1CURSOR, "p", cmd_extract
	}, {
		"y",            "As `x` but select unmatched text",
		CMD_CMD|CMD_REGEX|CMD_ADDRESS_ALL_1CURSOR, "p", cmd_extract
	}, {
		"X",            "Run command on files whose name matches",
		CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT|CMD_ADDRESS_NONE, NULL, cmd_files
	}, {
		"Y",            "As `X` but select unmatched files",
		CMD_CMD|CMD_REGEX|CMD_ADDRESS_NONE, NULL, cmd_files
	}, {
		">",            "Send range to stdin of command",
		CMD_SHELL|CMD_ADDRESS_LINE, NULL, cmd_pipeout
	}, {
		"<",            "Replace range by stdout of command",
		CMD_SHELL|CMD_ADDRESS_POS, NULL, cmd_pipein
	}, {
		"|",            "Pipe range through command",
		CMD_SHELL|CMD_ADDRESS_POS, NULL, cmd_filter
	}, {
		"!",            "Run the command",
		CMD_SHELL|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_launch
	}, {
		"w",            "Write range to named file",
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_ALL, NULL, cmd_write
	}, {
		"r",            "Replace range by contents of file",
		CMD_ARGV|CMD_ADDRESS_AFTER, NULL, cmd_read
	}, {
		"{",            "Start of command group",
		CMD_NONE, NULL, NULL
	}, {
		"}",            "End of command group" ,
		CMD_NONE, NULL, NULL
	}, {
		"e",            "Edit file",
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_edit
	}, {
		"q",            "Quit the current window",
		CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_quit
	}, {
		"cd",           "Change directory",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_cd
	},
	/* vi(m) related commands */
	{
		"bdelete",      "Unload file",
		CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_bdelete
	}, {
		"help",         "Show this help",
		CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_help
	}, {
		"map",          "Map key binding `:map <mode> <lhs> <rhs>`",
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_map
	}, {
		"map-window",   "As `map` but window local",
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_map
	}, {
		"unmap",        "Unmap key binding `:unmap <mode> <lhs>`",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_unmap
	}, {
		"unmap-window", "As `unmap` but window local",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_unmap
	}, {
		"langmap",      "Map keyboard layout `:langmap <locale-keys> <latin-keys>`",
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_langmap
	}, {
		"new",          "Create new window",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_new
	}, {
		"open",         "Open file",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_open
	}, {
		"qall",         "Exit vis",
		CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_qall
	}, {
		"set",          "Set option",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_set
	}, {
		"split",        "Horizontally split window",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_split
	}, {
		"vnew",         "As `:new` but split vertically",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_vnew
	}, {
		"vsplit",       "Vertically split window",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_vsplit
	}, {
		"wq",           "Write file and quit",
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_ALL, NULL, cmd_wq
	}, {
		"earlier",      "Go to older text state",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_earlier_later
	}, {
		"later",        "Go to newer text state",
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_earlier_later
	},
	{ NULL, NULL, CMD_NONE, NULL, NULL },
};

static const CommandDef cmddef_select = {
	NULL, NULL, CMD_NONE, NULL, cmd_select
};

static const CommandDef cmddef_user = {
	NULL, NULL, CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_ALL, NULL, cmd_user
};

/* :set command options */
typedef struct {
	const char *names[3];                  /* name and optional alias */
	enum {
		OPTION_TYPE_STRING,
		OPTION_TYPE_BOOL,
		OPTION_TYPE_NUMBER,
	} type;
	enum {
		OPTION_FLAG_NONE = 0,
		OPTION_FLAG_OPTIONAL = 1 << 0, /* value is optional */
		OPTION_FLAG_WINDOW = 1 << 1,   /* option requires an active window */
	} flags;
	const char *help;                      /* short, one line help text */
} OptionDef;

enum {
	OPTION_SHELL,
	OPTION_ESCDELAY,
	OPTION_AUTOINDENT,
	OPTION_EXPANDTAB,
	OPTION_TABWIDTH,
	OPTION_THEME,
	OPTION_SYNTAX,
	OPTION_SHOW_SPACES,
	OPTION_SHOW_TABS,
	OPTION_SHOW_NEWLINES,
	OPTION_NUMBER,
	OPTION_NUMBER_RELATIVE,
	OPTION_CURSOR_LINE,
	OPTION_COLOR_COLUMN,
	OPTION_HORIZON,
	OPTION_SAVE_METHOD,
};

static const OptionDef options[] = {
	[OPTION_SHELL] = {
		{ "shell" },
		OPTION_TYPE_STRING, OPTION_FLAG_NONE,
		"Shell to use for external commands (default: $SHELL, /etc/passwd, /bin/sh)",
	},
	[OPTION_ESCDELAY] = {
		{ "escdelay" },
		OPTION_TYPE_NUMBER, OPTION_FLAG_NONE,
		"Miliseconds to wait to distinguish <Escape> from terminal escape sequences",
	},
	[OPTION_AUTOINDENT] = {
		{ "autoindent", "ai" },
		OPTION_TYPE_BOOL, OPTION_FLAG_NONE,
		"Copy leading white space from previous line",
	},
	[OPTION_EXPANDTAB] = {
		{ "expandtab", "et" },
		OPTION_TYPE_BOOL, OPTION_FLAG_NONE,
		"Replace entered <Tab> with `tabwidth` spaces",
	},
	[OPTION_TABWIDTH] = {
		{ "tabwidth", "tw" },
		OPTION_TYPE_NUMBER, OPTION_FLAG_NONE,
		"Number of spaces to display (and insert if `expandtab` is enabled) for a tab",
	},
	[OPTION_THEME] = {
		{ "theme" },
		OPTION_TYPE_STRING, OPTION_FLAG_NONE,
		"Color theme to use filename without extension",
	},
	[OPTION_SYNTAX] = {
		{ "syntax" },
		OPTION_TYPE_STRING, OPTION_FLAG_WINDOW|OPTION_FLAG_OPTIONAL,
		"Syntax highlighting lexer to use filename without extension",
	},
	[OPTION_SHOW_SPACES] = {
		{ "show-spaces" },
		OPTION_TYPE_BOOL, OPTION_FLAG_WINDOW,
		"Display replacement symbol instead of a space",
	},
	[OPTION_SHOW_TABS] = {
		{ "show-tabs" },
		OPTION_TYPE_BOOL, OPTION_FLAG_WINDOW,
		"Display replacement symbol for tabs",
	},
	[OPTION_SHOW_NEWLINES] = {
		{ "show-newlines" },
		OPTION_TYPE_BOOL, OPTION_FLAG_WINDOW,
		"Display replacement symbol for newlines",
	},
	[OPTION_NUMBER] = {
		{ "numbers", "nu" },
		OPTION_TYPE_BOOL, OPTION_FLAG_WINDOW,
		"Display absolute line numbers",
	},
	[OPTION_NUMBER_RELATIVE] = {
		{ "relativenumbers", "rnu" },
		OPTION_TYPE_BOOL, OPTION_FLAG_WINDOW,
		"Display relative line numbers",
	},
	[OPTION_CURSOR_LINE] = {
		{ "cursorline", "cul" },
		OPTION_TYPE_BOOL, OPTION_FLAG_WINDOW,
		"Highlight current cursor line",
	},
	[OPTION_COLOR_COLUMN] = {
		{ "colorcolumn", "cc" },
		OPTION_TYPE_NUMBER, OPTION_FLAG_WINDOW,
		"Highlight a fixed column",
	},
	[OPTION_HORIZON] = {
		{ "horizon" },
		OPTION_TYPE_NUMBER, OPTION_FLAG_WINDOW,
		"Number of bytes to consider for syntax highlighting",
	},
	[OPTION_SAVE_METHOD] = {
		{ "savemethod" },
		OPTION_TYPE_STRING, OPTION_FLAG_WINDOW,
		"Save method to use for current file 'auto', 'atomic' or 'inplace'",
	},
};

bool sam_init(Vis *vis) {
	if (!(vis->cmds = map_new()))
		return false;
	bool ret = true;
	for (const CommandDef *cmd = cmds; cmd && cmd->name; cmd++)
		ret &= map_put(vis->cmds, cmd->name, cmd);

	if (!(vis->options = map_new()))
		return false;
	for (int i = 0; i < LENGTH(options); i++) {
		for (const char *const *name = options[i].names; *name; name++)
			ret &= map_put(vis->options, *name, &options[i]);
	}

	return ret;
}

const char *sam_error(enum SamError err) {
	static const char *error_msg[] = {
		[SAM_ERR_OK]              = "Success",
		[SAM_ERR_MEMORY]          = "Out of memory",
		[SAM_ERR_ADDRESS]         = "Bad address",
		[SAM_ERR_NO_ADDRESS]      = "Command takes no address",
		[SAM_ERR_UNMATCHED_BRACE] = "Unmatched `}'",
		[SAM_ERR_REGEX]           = "Bad regular expression",
		[SAM_ERR_TEXT]            = "Bad text",
		[SAM_ERR_SHELL]           = "Shell command expected",
		[SAM_ERR_COMMAND]         = "Unknown command",
		[SAM_ERR_EXECUTE]         = "Error executing command",
		[SAM_ERR_NEWLINE]         = "Newline expected",
		[SAM_ERR_MARK]            = "Invalid mark",
	};

	return err < LENGTH(error_msg) ? error_msg[err] : NULL;
}

static Address *address_new(void) {
	Address *addr = calloc(1, sizeof *addr);
	if (addr)
		addr->number = EPOS;
	return addr;
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

static char *parse_until(const char **s, const char *until, const char *escchars, int type){
	Buffer buf;
	buffer_init(&buf);
	size_t len = strlen(until);
	bool escaped = false;

	for (; **s && (!memchr(until, **s, len) || escaped); (*s)++) {
		if (type != CMD_SHELL && !escaped && **s == '\\') {
			escaped = true;
			continue;
		}

		char c = **s;

		if (escaped) {
			escaped = false;
			if (c == '\n')
				continue;
			if (c == 'n') {
				c = '\n';
			} else if (c == 't') {
				c = '\t';
			} else if (type != CMD_REGEX && c == '\\') {
				// ignore one of the back slashes
			} else {
				bool delim = memchr(until, c, len);
				bool esc = escchars && memchr(escchars, c, strlen(escchars));
				if (!delim && !esc)
					buffer_append(&buf, "\\", 1);
			}
		}

		if (!buffer_append(&buf, &c, 1)) {
			buffer_release(&buf);
			return NULL;
		}
	}

	if (buffer_length(&buf))
	    buffer_append(&buf, "\0", 1);

	return buf.data;
}

static char *parse_delimited(const char **s, int type) {
	char delim[2] = { **s, '\0' };
	if (!delim[0])
		return NULL;
	(*s)++;
	char *chunk = parse_until(s, delim, NULL, type);
	if (**s == delim[0])
		(*s)++;
	return chunk;
}

static char *parse_text(const char **s) {
	skip_spaces(s);
	if (**s != '\n')
		return parse_delimited(s, CMD_TEXT);

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

static char *parse_shellcmd(Vis *vis, const char **s) {
	skip_spaces(s);
	char *cmd = parse_until(s, "\n", NULL, false);
	if (!cmd) {
		const char *last_cmd = register_get(vis, &vis->registers[VIS_REG_SHELL], NULL);
		return last_cmd ? strdup(last_cmd) : NULL;
	}
	register_put0(vis, &vis->registers[VIS_REG_SHELL], cmd);
	return cmd;
}

static void parse_argv(const char **s, const char *argv[], size_t maxarg) {
	for (size_t i = 0; i < maxarg; i++) {
		skip_spaces(s);
		if (**s == '"' || **s == '\'')
			argv[i] = parse_delimited(s, CMD_ARGV);
		else
			argv[i] = parse_until(s, " \t\n", "\'\"", CMD_ARGV);
	}
}

static char *parse_cmdname(const char **s) {
	Buffer buf;
	buffer_init(&buf);

	skip_spaces(s);
	while (**s && !isspace((unsigned char)**s) && (!ispunct((unsigned char)**s) || **s == '-'))
		buffer_append(&buf, (*s)++, 1);

	if (buffer_length(&buf))
	    buffer_append(&buf, "\0", 1);

	return buf.data;
}

static Regex *parse_regex(Vis *vis, const char **s) {
	char *pattern = parse_delimited(s, CMD_REGEX);
	Regex *regex = vis_regex(vis, pattern);
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
		.number = EPOS,
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
	case '`':
		(*s)++;
		if ((addr.number = vis_mark_from(vis, **s)) == VIS_MARK_INVALID) {
			*err = SAM_ERR_MARK;
			return NULL;
		}
		(*s)++;
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
	return map_closest(vis->cmds, name);
}

static Command *command_parse(Vis *vis, const char **s, enum SamError *err) {
	if (!**s) {
		*err = SAM_ERR_COMMAND;
		return NULL;
	}
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
		int level = vis->nesting_level++;
		do {
			while (**s == ' ' || **s == '\t' || **s == '\n')
				(*s)++;
			next = command_parse(vis, s, err);
			if (prev)
				prev->next = next;
			else
				cmd->cmd = next;
		} while ((prev = next));
		if (level != vis->nesting_level) {
			*err = SAM_ERR_UNMATCHED_BRACE;
			goto fail;
		}
	} else if (strcmp(cmd->argv[0], "}") == 0) {
		if (vis->nesting_level-- == 0) {
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

	if (cmddef->flags & CMD_REGEX) {
		if ((cmddef->flags & CMD_REGEX_DEFAULT) && (!**s || **s == ' ')) {
			skip_spaces(s);
		} else if (!(cmd->regex = parse_regex(vis, s))) {
			*err = SAM_ERR_REGEX;
			goto fail;
		}
	}

	if (cmddef->flags & CMD_SHELL && !(cmd->argv[1] = parse_shellcmd(vis, s))) {
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
		if (cmddef->defcmd && (**s == '\n' || **s == '}' || **s == '\0')) {
			if (**s == '\n')
				(*s)++;
			if (!(cmd->cmd = command_new(cmddef->defcmd)))
				goto fail;
			cmd->cmd->cmddef = command_lookup(vis, cmddef->defcmd);
		} else {
			if (!(cmd->cmd = command_parse(vis, s, err)))
				goto fail;
			if (strcmp(cmd->argv[0], "X") == 0 || strcmp(cmd->argv[0], "Y") == 0) {
				Command *sel = command_new("select");
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
	vis->nesting_level = 0;
	const char **s = &cmd;
	Command *c = command_parse(vis, s, err);
	if (!c)
		return NULL;
	while (**s == ' ' || **s == '\t' || **s == '\n')
		(*s)++;
	if (**s) {
		*err = SAM_ERR_NEWLINE;
		command_free(c);
		return NULL;
	}

	Command *sel = command_new("select");
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
	size_t offset = addr->number != EPOS ? addr->number : 1;
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
		if (addr->number == 0)
			return text_range_new(0, 0);
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
		case '`':
		{
			size_t pos = text_mark_get(file->text, file->marks[addr->number]);
			ret = text_range_new(pos, pos);
			break;
		}
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
	if (cmd->address && win)
		*range = address_evaluate(cmd->address, win->file, range, 0);

	switch (cmd->argv[0][0]) {
	case '{':
	{
		if (!win) {
			ret = false;
			break;
		}
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

		view_cursors_dispose(cur);
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

	if (vis->win) {
		bool completed = true;
		for (Cursor *c = view_cursors(vis->win->view); c; c = view_cursors_next(c)) {
			Filerange sel = view_cursors_selection_get(c);
			if (text_range_valid(&sel)) {
				completed = false;
				break;
			}
		}
		vis_mode_switch(vis, completed ? VIS_MODE_NORMAL : VIS_MODE_VISUAL);
	}
	command_free(cmd);
	return err;
}

static bool cmd_insert(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	size_t len = strlen(argv[1]);
	bool ret = text_insert(win->file->text, range->start, argv[1], len);
	if (ret) {
		*range = text_range_new(range->start, range->start + len);
		if (cur)
			view_cursors_to(cur, range->end);
	}
	return ret;
}

static bool cmd_append(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	size_t len = strlen(argv[1]);
	bool ret = text_insert(win->file->text, range->end, argv[1], len);
	if (ret) {
		*range = text_range_new(range->end, range->end + len);
		if (cur)
			view_cursors_to(cur, range->end);
	}
	return ret;
}

static bool cmd_change(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	Text *txt = win->file->text;
	size_t len = strlen(argv[1]);
	bool ret = text_delete(txt, range->start, text_range_size(range)) &&
	      text_insert(txt, range->start, argv[1], len);
	if (ret) {
		*range = text_range_new(range->start, range->start + len);
		if (cur)
			view_cursors_to(cur, range->end);
	}
	return ret;
}

static bool cmd_delete(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	bool ret = text_delete(win->file->text, range->start, text_range_size(range));
	if (ret) {
		*range = text_range_new(range->start, range->start);
		if (cur)
			view_cursors_to(cur, range->end);
	}
	return ret;
}

static bool cmd_guard(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	bool match = !text_search_range_forward(win->file->text, range->start,
		text_range_size(range), cmd->regex, 0, NULL, 0);
	if (match ^ (argv[0][0] == 'v'))
		return sam_execute(vis, win, cmd->cmd, cur, range);
	if (cur && !view_cursors_dispose(cur))
		view_cursors_selection_clear(cur);
	return true;
}

static bool cmd_extract(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	bool ret = true;
	size_t pos = EPOS;
	Text *txt = win->file->text;

	if (cmd->regex) {
		bool trailing_match = false;
		size_t start = range->start, end = range->end, last_start = EPOS;
		RegexMatch match[1];
		while (start < end || trailing_match) {
			trailing_match = false;
			bool found = text_search_range_forward(txt, start,
				end - start, cmd->regex, 1, match,
				start > range->start ? REG_NOTBOL : 0) == 0;
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
					/* in Plan 9's regexp library ^ matches the beginning
					 * of a line, however in POSIX with REG_NEWLINE ^
					 * matches the zero-length string immediately after a
					 * newline. Try filtering out the last such match at EOF.
					 */
					if (end == match[0].start && start > range->start)
						break;
				}
				start = match[0].end;
				if (start == end)
					trailing_match = true;
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
				if (start == EPOS)
					last_start = start = r.end;
				if (ret && pos == EPOS)
					pos = start;
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
			if (ret && pos == EPOS)
				pos = start;
			end = text_mark_get(txt, mark_end);
			if (end == EPOS) {
				ret = false;
				break;
			}
		}
	}

	if (cur && !view_cursors_dispose(cur)) {
		view_cursors_selection_clear(cur);
		view_cursors_to(cur, pos != EPOS ? pos : range->start);
	}
	return ret;
}

static bool cmd_select(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	Filerange sel = text_range_empty();
	if (!win)
		return sam_execute(vis, NULL, cmd->cmd, NULL, &sel);
	bool ret = true;
	View *view = win->view;
	Text *txt = win->file->text;
	bool multiple_cursors = view_cursors_multiple(view);
	Cursor *primary = view_cursors_primary_get(view);

	for (Cursor *c = view_cursors(view), *next; c && ret; c = next) {
		next = view_cursors_next(c);
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
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_ALL) {
			sel = text_range_new(0, text_size(txt));
		} else if (!multiple_cursors && (cmd->cmd->cmddef->flags & CMD_ADDRESS_ALL_1CURSOR)) {
			sel = text_range_new(0, text_size(txt));
		} else {
			sel = text_range_new(pos, text_char_next(txt, pos));
		}
		if (text_range_valid(&sel))
			ret &= sam_execute(vis, win, cmd->cmd, c, &sel);
		if (cmd->cmd->cmddef->flags & CMD_ONCE)
			break;
	}

	if (vis->win && vis->win->view == view && primary != view_cursors_primary_get(view))
		view_cursors_primary_set(view_cursors(view));
	return ret;
}

static bool cmd_print(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win || !text_range_valid(range))
		return false;
	View *view = win->view;
	Text *txt = win->file->text;
	size_t pos = range->end;
	if (range->start != range->end)
		pos = text_char_prev(txt, pos);
	if (cur)
		view_cursors_to(cur, pos);
	else
		cur = view_cursors_new_force(view, pos);
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

static bool cmd_write(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *r) {
	if (!win)
		return false;
	File *file = win->file;
	Text *text = file->text;
	bool noname = !argv[1];
	if (!argv[1])
		argv[1] = file->name ? strdup(file->name) : NULL;
	if (!argv[1]) {
		if (file->fd == -1) {
			vis_info_show(vis, "Filename expected");
			return false;
		}
		if (!strchr(argv[0], 'q')) {
			vis_info_show(vis, "No filename given, use 'wq' to write to stdout");
			return false;
		}

		if (!vis_event_emit(vis, VIS_EVENT_FILE_SAVE_PRE, file, (char*)NULL) && cmd->flags != '!') {
			vis_info_show(vis, "Rejected write to stdout by pre-save hook");
			return false;
		}

		for (Cursor *c = view_cursors(win->view); c; c = view_cursors_next(c)) {
			Filerange range = view_cursors_selection_get(c);
			bool invalid_range = !text_range_valid(&range);
			if (invalid_range)
				range = *r;
			ssize_t written = text_write_range(text, &range, file->fd);
			if (written == -1 || (size_t)written != text_range_size(&range)) {
				vis_info_show(vis, "Can not write to stdout");
				return false;
			}
			if (invalid_range)
				break;
		}

		/* make sure the file is marked as saved i.e. not modified */
		text_save(text, NULL);
		vis_event_emit(vis, VIS_EVENT_FILE_SAVE_POST, file, (char*)NULL);
		return true;
	}

	if (noname && cmd->flags != '!') {
		if (vis->mode->visual) {
			vis_info_show(vis, "WARNING: file will be reduced to active selection");
			return false;
		}
		Filerange all = text_range_new(0, text_size(text));
		if (!text_range_equal(r, &all)) {
			vis_info_show(vis, "WARNING: file will be reduced to provided range");
			return false;
		}
	}

	for (const char **name = &argv[1]; *name; name++) {
		struct stat meta;
		if (cmd->flags != '!' && file->stat.st_mtime && stat(*name, &meta) == 0 &&
		    file->stat.st_mtime < meta.st_mtime) {
			vis_info_show(vis, "WARNING: file has been changed since reading it");
			return false;
		}

		if (!vis_event_emit(vis, VIS_EVENT_FILE_SAVE_PRE, file, *name) && cmd->flags != '!') {
			vis_info_show(vis, "Rejected write to `%s' by pre-save hook", *name);
			return false;
		}

		TextSave *ctx = text_save_begin(text, *name, file->save_method);
		if (!ctx) {
			const char *msg = errno ? strerror(errno) : "try changing `:set savemethod`";
			vis_info_show(vis, "Can't write `%s': %s", *name, msg);
			return false;
		}

		bool failure = false;

		for (Cursor *c = view_cursors(win->view); c; c = view_cursors_next(c)) {
			Filerange range = view_cursors_selection_get(c);
			bool invalid_range = !text_range_valid(&range);
			if (invalid_range)
				range = *r;

			ssize_t written = text_save_write_range(ctx, &range);
			failure = (written == -1 || (size_t)written != text_range_size(&range));
			if (failure) {
				text_save_cancel(ctx);
				break;
			}

			if (invalid_range)
				break;
		}

		if (failure || !text_save_commit(ctx)) {
			vis_info_show(vis, "Can't write `%s': %s", *name, strerror(errno));
			return false;
		}

		if (!file->name)
			file_name_set(file, *name);
		if (strcmp(file->name, *name) == 0)
			file->stat = text_stat(text);
		vis_event_emit(vis, VIS_EVENT_FILE_SAVE_POST, file, *name);
	}
	return true;
}

static bool cmd_read(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!argv[1]) {
		vis_info_show(vis, "Filename expected");
		return false;
	}

	const char *args[MAX_ARGV] = { argv[0], "cat", "--" };
	for (int i = 3; i < MAX_ARGV-2; i++)
		args[i] = argv[i-2];
	args[MAX_ARGV-1] = NULL;
	return cmd_pipein(vis, win, cmd, (const char**)args, cur, range);
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
	if (!win)
		return false;
	Text *txt = win->file->text;

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
		range->end = filter.pos - text_range_size(range);
		if (cur)
			view_cursors_to(cur, range->start);
	} else {
		text_delete(txt, range->end, filter.pos - range->end);
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
	Filerange invalid = text_range_new(cur ? view_cursors_pos(cur) : range->start, EPOS);
	return cmd_filter(vis, win, cmd, argv, cur, &invalid);
}

static bool cmd_pipein(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
	Filerange filter_range = text_range_new(range->end, range->end);
	bool ret = cmd_filter(vis, win, cmd, argv, cur, &filter_range);
	if (ret) {
		text_delete_range(win->file->text, range);
		range->end = range->start + text_range_size(&filter_range);
		if (cur)
			view_cursors_to(cur, range->start);
	}
	return ret;
}

static bool cmd_pipeout(Vis *vis, Win *win, Command *cmd, const char *argv[], Cursor *cur, Filerange *range) {
	if (!win)
		return false;
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
