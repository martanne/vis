/*
 * Heavily inspired (and partially based upon) the X11 version of
 * Rob Pike's sam text editor originally written for Plan 9.
 *
 *  Copyright © 2016-2020 Marc André Tanner <mat at brain-dump.org>
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
#include <fcntl.h>
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

struct Change {
	enum ChangeType {
		TRANSCRIPT_INSERT = 1 << 0,
		TRANSCRIPT_DELETE = 1 << 1,
		TRANSCRIPT_CHANGE = TRANSCRIPT_INSERT|TRANSCRIPT_DELETE,
	} type;
	Win *win;          /* window in which changed file is being displayed */
	Selection *sel;    /* selection associated with this change, might be NULL */
	Filerange range;   /* inserts are denoted by zero sized range (same start/end) */
	const char *data;  /* will be free(3)-ed after transcript has been processed */
	size_t len;        /* size in bytes of the chunk pointed to by data */
	Change *next;      /* modification position increase monotonically */
	int count;         /* how often should data be inserted? */
};

struct Address {
	char type;      /* # (char) l (line) g (goto line) / ? . $ + - , ; % ' */
	Regex *regex;   /* NULL denotes default for x, y, X, and Y commands */
	size_t number;  /* line or character number */
	Address *left;  /* left hand side of a compound address , ; */
	Address *right; /* either right hand side of a compound address or next address */
};

typedef struct {
	int start, end; /* interval [n,m] */
	bool mod;       /* % every n-th match, implies n == m */
} Count;

struct Command {
	const char *argv[MAX_ARGV];/* [0]=cmd-name, [1..MAX_ARGV-2]=arguments, last element always NULL */
	Address *address;         /* range of text for command */
	Regex *regex;             /* regex to match, used by x, y, g, v, X, Y */
	const CommandDef *cmddef; /* which command is this? */
	Count count;              /* command count, defaults to [0,+inf] */
	int iteration;            /* current command loop iteration */
	char flags;               /* command specific flags */
	Command *cmd;             /* target of x, y, g, v, X, Y, { */
	Command *next;            /* next command in {} group */
};

struct CommandDef {
	const char *name;                    /* command name */
	VIS_HELP_DECL(const char *help;)     /* short, one-line help text */
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
		CMD_LOOP          = 1 << 15, /* a looping construct like `x`, `y` */
		CMD_GROUP         = 1 << 16, /* a command group { ... } */
		CMD_DESTRUCTIVE   = 1 << 17, /* command potentially destroys window */
	} flags;
	const char *defcmd;                  /* name of a default target command */
	bool (*func)(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*); /* command implementation */
};

/* sam commands */
static bool cmd_insert(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_append(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_change(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_delete(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_guard(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_extract(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_select(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_print(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_files(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_pipein(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_pipeout(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_filter(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_launch(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_substitute(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_write(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_read(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_edit(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_quit(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_cd(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
/* vi(m) commands */
static bool cmd_set(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_open(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_qall(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_split(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_vsplit(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_new(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_vnew(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_wq(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_earlier_later(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_help(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_map(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_unmap(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_langmap(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);
static bool cmd_user(Vis*, Win*, Command*, const char *argv[], Selection*, Filerange*);

static const CommandDef cmds[] = {
	//      name            help
	//      flags, default command, implemenation
	{
		"a",            VIS_HELP("Append text after range")
		CMD_TEXT, NULL, cmd_append
	}, {
		"c",            VIS_HELP("Change text in range")
		CMD_TEXT, NULL, cmd_change
	}, {
		"d",            VIS_HELP("Delete text in range")
		CMD_NONE, NULL, cmd_delete
	}, {
		"g",            VIS_HELP("If range contains regexp, run command")
		CMD_COUNT|CMD_REGEX|CMD_CMD, "p", cmd_guard
	}, {
		"i",            VIS_HELP("Insert text before range")
		CMD_TEXT, NULL, cmd_insert
	}, {
		"p",            VIS_HELP("Create selection covering range")
		CMD_NONE, NULL, cmd_print
	}, {
		"s",            VIS_HELP("Substitute: use x/pattern/ c/replacement/ instead")
		CMD_SHELL|CMD_ADDRESS_LINE, NULL, cmd_substitute
	}, {
		"v",            VIS_HELP("If range does not contain regexp, run command")
		CMD_COUNT|CMD_REGEX|CMD_CMD, "p", cmd_guard
	}, {
		"x",            VIS_HELP("Set range and run command on each match")
		CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT|CMD_ADDRESS_ALL_1CURSOR|CMD_LOOP, "p", cmd_extract
	}, {
		"y",            VIS_HELP("As `x` but select unmatched text")
		CMD_CMD|CMD_REGEX|CMD_ADDRESS_ALL_1CURSOR|CMD_LOOP, "p", cmd_extract
	}, {
		"X",            VIS_HELP("Run command on files whose name matches")
		CMD_CMD|CMD_REGEX|CMD_REGEX_DEFAULT|CMD_ADDRESS_NONE|CMD_ONCE, NULL, cmd_files
	}, {
		"Y",            VIS_HELP("As `X` but select unmatched files")
		CMD_CMD|CMD_REGEX|CMD_ADDRESS_NONE|CMD_ONCE, NULL, cmd_files
	}, {
		">",            VIS_HELP("Send range to stdin of command")
		CMD_SHELL|CMD_ADDRESS_LINE, NULL, cmd_pipeout
	}, {
		"<",            VIS_HELP("Replace range by stdout of command")
		CMD_SHELL|CMD_ADDRESS_POS, NULL, cmd_pipein
	}, {
		"|",            VIS_HELP("Pipe range through command")
		CMD_SHELL, NULL, cmd_filter
	}, {
		"!",            VIS_HELP("Run the command")
		CMD_SHELL|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_launch
	}, {
		"w",            VIS_HELP("Write range to named file")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_ALL, NULL, cmd_write
	}, {
		"r",            VIS_HELP("Replace range by contents of file")
		CMD_ARGV|CMD_ADDRESS_AFTER, NULL, cmd_read
	}, {
		"{",            VIS_HELP("Start of command group")
		CMD_GROUP, NULL, NULL
	}, {
		"}",            VIS_HELP("End of command group" )
		CMD_NONE, NULL, NULL
	}, {
		"e",            VIS_HELP("Edit file")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE|CMD_DESTRUCTIVE, NULL, cmd_edit
	}, {
		"q",            VIS_HELP("Quit the current window")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE|CMD_DESTRUCTIVE, NULL, cmd_quit
	}, {
		"cd",           VIS_HELP("Change directory")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_cd
	},
	/* vi(m) related commands */
	{
		"help",         VIS_HELP("Show this help")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_help
	}, {
		"map",          VIS_HELP("Map key binding `:map <mode> <lhs> <rhs>`")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_map
	}, {
		"map-window",   VIS_HELP("As `map` but window local")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_map
	}, {
		"unmap",        VIS_HELP("Unmap key binding `:unmap <mode> <lhs>`")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_unmap
	}, {
		"unmap-window", VIS_HELP("As `unmap` but window local")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_unmap
	}, {
		"langmap",      VIS_HELP("Map keyboard layout `:langmap <locale-keys> <latin-keys>`")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_langmap
	}, {
		"new",          VIS_HELP("Create new window")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_new
	}, {
		"open",         VIS_HELP("Open file")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_open
	}, {
		"qall",         VIS_HELP("Exit vis")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_NONE|CMD_DESTRUCTIVE, NULL, cmd_qall
	}, {
		"set",          VIS_HELP("Set option")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_set
	}, {
		"split",        VIS_HELP("Horizontally split window")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_split
	}, {
		"vnew",         VIS_HELP("As `:new` but split vertically")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_vnew
	}, {
		"vsplit",       VIS_HELP("Vertically split window")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_vsplit
	}, {
		"wq",           VIS_HELP("Write file and quit")
		CMD_ARGV|CMD_FORCE|CMD_ONCE|CMD_ADDRESS_ALL|CMD_DESTRUCTIVE, NULL, cmd_wq
	}, {
		"earlier",      VIS_HELP("Go to older text state")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_earlier_later
	}, {
		"later",        VIS_HELP("Go to newer text state")
		CMD_ARGV|CMD_ONCE|CMD_ADDRESS_NONE, NULL, cmd_earlier_later
	},
	{ NULL, VIS_HELP(NULL) CMD_NONE, NULL, NULL },
};

static const CommandDef cmddef_select = {
	NULL, VIS_HELP(NULL) CMD_NONE, NULL, cmd_select
};

/* :set command options */
typedef struct {
	const char *names[3];            /* name and optional alias */
	enum VisOption flags;            /* option type, etc. */
	VIS_HELP_DECL(const char *help;) /* short, one line help text */
	VisOptionFunction *func;         /* option handler, NULL for bulitins */
	void *context;                   /* context passed to option handler function */
} OptionDef;

enum {
	OPTION_SHELL,
	OPTION_ESCDELAY,
	OPTION_AUTOINDENT,
	OPTION_EXPANDTAB,
	OPTION_TABWIDTH,
	OPTION_SHOW_SPACES,
	OPTION_SHOW_TABS,
	OPTION_SHOW_NEWLINES,
	OPTION_SHOW_EOF,
	OPTION_NUMBER,
	OPTION_NUMBER_RELATIVE,
	OPTION_CURSOR_LINE,
	OPTION_COLOR_COLUMN,
	OPTION_SAVE_METHOD,
	OPTION_LOAD_METHOD,
	OPTION_CHANGE_256COLORS,
	OPTION_LAYOUT,
	OPTION_IGNORECASE,
};

static const OptionDef options[] = {
	[OPTION_SHELL] = {
		{ "shell" },
		VIS_OPTION_TYPE_STRING,
		VIS_HELP("Shell to use for external commands (default: $SHELL, /etc/passwd, /bin/sh)")
	},
	[OPTION_ESCDELAY] = {
		{ "escdelay" },
		VIS_OPTION_TYPE_NUMBER,
		VIS_HELP("Milliseconds to wait to distinguish <Escape> from terminal escape sequences")
	},
	[OPTION_AUTOINDENT] = {
		{ "autoindent", "ai" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Copy leading white space from previous line")
	},
	[OPTION_EXPANDTAB] = {
		{ "expandtab", "et" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Replace entered <Tab> with `tabwidth` spaces")
	},
	[OPTION_TABWIDTH] = {
		{ "tabwidth", "tw" },
		VIS_OPTION_TYPE_NUMBER,
		VIS_HELP("Number of spaces to display (and insert if `expandtab` is enabled) for a tab")
	},
	[OPTION_SHOW_SPACES] = {
		{ "show-spaces" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol instead of a space")
	},
	[OPTION_SHOW_TABS] = {
		{ "show-tabs" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol for tabs")
	},
	[OPTION_SHOW_NEWLINES] = {
		{ "show-newlines" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol for newlines")
	},
	[OPTION_SHOW_EOF] = {
		{ "show-eof" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display replacement symbol for lines after the end of the file")
	},
	[OPTION_NUMBER] = {
		{ "numbers", "nu" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display absolute line numbers")
	},
	[OPTION_NUMBER_RELATIVE] = {
		{ "relativenumbers", "rnu" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Display relative line numbers")
	},
	[OPTION_CURSOR_LINE] = {
		{ "cursorline", "cul" },
		VIS_OPTION_TYPE_BOOL|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Highlight current cursor line")
	},
	[OPTION_COLOR_COLUMN] = {
		{ "colorcolumn", "cc" },
		VIS_OPTION_TYPE_NUMBER|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Highlight a fixed column")
	},
	[OPTION_SAVE_METHOD] = {
		{ "savemethod" },
		VIS_OPTION_TYPE_STRING|VIS_OPTION_NEED_WINDOW,
		VIS_HELP("Save method to use for current file 'auto', 'atomic' or 'inplace'")
	},
	[OPTION_LOAD_METHOD] = {
		{ "loadmethod" },
		VIS_OPTION_TYPE_STRING,
		VIS_HELP("How to load existing files 'auto', 'read' or 'mmap'")
	},
	[OPTION_CHANGE_256COLORS] = {
		{ "change-256colors" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Change 256 color palette to support 24bit colors")
	},
	[OPTION_LAYOUT] = {
		{ "layout" },
		VIS_OPTION_TYPE_STRING,
		VIS_HELP("Vertical or horizontal window layout")
	},
	[OPTION_IGNORECASE] = {
		{ "ignorecase", "ic" },
		VIS_OPTION_TYPE_BOOL,
		VIS_HELP("Ignore case when searching")
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
		[SAM_ERR_CONFLICT]        = "Conflicting changes",
		[SAM_ERR_WRITE_CONFLICT]  = "Can not write while changing",
		[SAM_ERR_LOOP_INVALID_CMD]  = "Destructive command in looping construct",
		[SAM_ERR_GROUP_INVALID_CMD] = "Destructive command in group",
		[SAM_ERR_COUNT]           = "Invalid count",
	};

	size_t idx = err;
	return idx < LENGTH(error_msg) ? error_msg[idx] : NULL;
}

static void change_free(Change *c) {
	if (!c)
		return;
	free((char*)c->data);
	free(c);
}

static Change *change_new(Transcript *t, enum ChangeType type, Filerange *range, Win *win, Selection *sel) {
	if (!text_range_valid(range))
		return NULL;
	Change **prev, *next;
	if (t->latest && t->latest->range.end <= range->start) {
		prev = &t->latest->next;
		next = t->latest->next;
	} else {
		prev = &t->changes;
		next = t->changes;
	}
	while (next && next->range.end <= range->start) {
		prev = &next->next;
		next = next->next;
	}
	if (next && next->range.start < range->end) {
		t->error = SAM_ERR_CONFLICT;
		return NULL;
	}
	Change *new = calloc(1, sizeof *new);
	if (new) {
		new->type = type;
		new->range = *range;
		new->sel = sel;
		new->win = win;
		new->next = next;
		*prev = new;
		t->latest = new;
	}
	return new;
}

static void sam_transcript_init(Transcript *t) {
	memset(t, 0, sizeof *t);
}

static bool sam_transcript_error(Transcript *t, enum SamError error) {
	if (t->changes)
		t->error = error;
	return t->error;
}

static void sam_transcript_free(Transcript *t) {
	for (Change *c = t->changes, *next; c; c = next) {
		next = c->next;
		change_free(c);
	}
}

static bool sam_insert(Win *win, Selection *sel, size_t pos, const char *data, size_t len, int count) {
	Filerange range = text_range_new(pos, pos);
	Change *c = change_new(&win->file->transcript, TRANSCRIPT_INSERT, &range, win, sel);
	if (c) {
		c->data = data;
		c->len = len;
		c->count = count;
	}
	return c;
}

static bool sam_delete(Win *win, Selection *sel, Filerange *range) {
	return change_new(&win->file->transcript, TRANSCRIPT_DELETE, range, win, sel);
}

static bool sam_change(Win *win, Selection *sel, Filerange *range, const char *data, size_t len, int count) {
	Change *c = change_new(&win->file->transcript, TRANSCRIPT_CHANGE, range, win, sel);
	if (c) {
		c->data = data;
		c->len = len;
		c->count = count;
	}
	return c;
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
			} else if (type != CMD_REGEX && type != CMD_TEXT && c == '\\') {
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

	buffer_terminate(&buf);

	return buffer_move(&buf);
}

static char *parse_delimited(const char **s, int type) {
	char delim[2] = { **s, '\0' };
	if (!delim[0] || isspace((unsigned char)delim[0]))
		return NULL;
	(*s)++;
	char *chunk = parse_until(s, delim, NULL, type);
	if (**s == delim[0])
		(*s)++;
	return chunk;
}

static int parse_number(const char **s) {
	char *end = NULL;
	int number = strtoull(*s, &end, 10);
	if (end == *s)
		return 0;
	*s = end;
	return number;
}

static char *parse_text(const char **s, Count *count) {
	skip_spaces(s);
	const char *before = *s;
	count->start = parse_number(s);
	if (*s == before)
		count->start = 1;
	if (**s != '\n') {
		before = *s;
		char *text = parse_delimited(s, CMD_TEXT);
		return (!text && *s != before) ? strdup("") : text;
	}

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

	return buffer_move(&buf);
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

static bool valid_cmdname(const char *s) {
	unsigned char c = (unsigned char)*s;
	return c && !isspace(c) && !isdigit(c) && (!ispunct(c) || (c == '-' && valid_cmdname(s+1)));
}

static char *parse_cmdname(const char **s) {
	Buffer buf;
	buffer_init(&buf);

	skip_spaces(s);
	while (valid_cmdname(*s))
		buffer_append(&buf, (*s)++, 1);

	buffer_terminate(&buf);

	return buffer_move(&buf);
}

static Regex *parse_regex(Vis *vis, const char **s) {
	const char *before = *s;
	char *pattern = parse_delimited(s, CMD_REGEX);
	if (!pattern && *s == before)
		return NULL;
	Regex *regex = vis_regex(vis, pattern);
	free(pattern);
	return regex;
}

static enum SamError parse_count(const char **s, Count *count) {
	count->mod = **s == '%';

	if (count->mod) {
		(*s)++;
		int n = parse_number(s);
		if (!n)
			return SAM_ERR_COUNT;
		count->start = n;
		count->end = n;
		return SAM_ERR_OK;
	}

	const char *before = *s;
	if (!(count->start = parse_number(s)) && *s != before)
		return SAM_ERR_COUNT;
	if (**s != ',') {
		count->end = count->start ? count->start : INT_MAX;
		return SAM_ERR_OK;
	} else {
		(*s)++;
	}
	before = *s;
	if (!(count->end = parse_number(s)) && *s != before)
		return SAM_ERR_COUNT;
	if (!count->end)
		count->end = INT_MAX;
	return SAM_ERR_OK;
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
	case '\'':
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
			if (*err)
				goto fail;
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

	if (cmddef->flags & CMD_FORCE && **s == '!') {
		cmd->flags = '!';
		(*s)++;
	}

	if ((cmddef->flags & CMD_COUNT) && (*err = parse_count(s, &cmd->count)))
		goto fail;

	if (cmddef->flags & CMD_REGEX) {
		if ((cmddef->flags & CMD_REGEX_DEFAULT) && (!**s || **s == ' ')) {
			skip_spaces(s);
		} else {
			const char *before = *s;
			cmd->regex = parse_regex(vis, s);
			if (!cmd->regex && (*s != before || !(cmddef->flags & CMD_COUNT))) {
				*err = SAM_ERR_REGEX;
				goto fail;
			}
		}
	}

	if (cmddef->flags & CMD_SHELL && !(cmd->argv[1] = parse_shellcmd(vis, s))) {
		*err = SAM_ERR_SHELL;
		goto fail;
	}

	if (cmddef->flags & CMD_TEXT && !(cmd->argv[1] = parse_text(s, &cmd->count))) {
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
		if (start < end && text_byte_get(txt, end-1, &c) && c == '\n')
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

static Filerange address_evaluate(Address *addr, File *file, Selection *sel, Filerange *range, int sign) {
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
		case '\'':
		{
			size_t pos = EPOS;
			Array *marks = &file->marks[addr->number];
			size_t idx = sel ? view_selections_number(sel) : 0;
			SelectionRegion *sr = array_get(marks, idx);
			if (sr)
				pos = text_mark_get(file->text, sr->cursor);
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
				left = address_evaluate(addr->left, file, sel, range, 0);
			else
				left = text_range_new(0, 0);

			if (addr->type == ';')
				range = &left;

			if (addr->right) {
				right = address_evaluate(addr->right, file, sel, range, 0);
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

static bool count_evaluate(Command *cmd) {
	Count *count = &cmd->count;
	if (count->mod)
		return count->start ? cmd->iteration % count->start == 0 : true;
	return count->start <= cmd->iteration && cmd->iteration <= count->end;
}

static bool sam_execute(Vis *vis, Win *win, Command *cmd, Selection *sel, Filerange *range) {
	bool ret = true;
	if (cmd->address && win)
		*range = address_evaluate(cmd->address, win->file, sel, range, 0);

	cmd->iteration++;
	switch (cmd->argv[0][0]) {
	case '{':
	{
		for (Command *c = cmd->cmd; c && ret; c = c->next)
			ret &= sam_execute(vis, win, c, NULL, range);
		view_selections_dispose_force(sel);
		break;
	}
	default:
		ret = cmd->cmddef->func(vis, win, cmd, cmd->argv, sel, range);
		break;
	}
	return ret;
}

static enum SamError validate(Command *cmd, bool loop, bool group) {
	if (cmd->cmddef->flags & CMD_DESTRUCTIVE) {
		if (loop)
			return SAM_ERR_LOOP_INVALID_CMD;
		if (group)
			return SAM_ERR_GROUP_INVALID_CMD;
	}

	group |= (cmd->cmddef->flags & CMD_GROUP);
	loop  |= (cmd->cmddef->flags & CMD_LOOP);
	for (Command *c = cmd->cmd; c; c = c->next) {
		enum SamError err = validate(c, loop, group);
		if (err != SAM_ERR_OK)
			return err;
	}
	return SAM_ERR_OK;
}

static enum SamError command_validate(Command *cmd) {
	return validate(cmd, false, false);
}

static bool count_negative(Command *cmd) {
	if (cmd->count.start < 0 || cmd->count.end < 0)
		return true;
	for (Command *c = cmd->cmd; c; c = c->next) {
		if (c->cmddef->func != cmd_extract && c->cmddef->func != cmd_select) {
			if (count_negative(c))
				return true;
		}
	}
	return false;
}

static void count_init(Command *cmd, int max) {
	Count *count = &cmd->count;
	cmd->iteration = 0;
	if (count->start < 0)
		count->start += max;
	if (count->end < 0)
		count->end += max;
	for (Command *c = cmd->cmd; c; c = c->next) {
		if (c->cmddef->func != cmd_extract && c->cmddef->func != cmd_select)
			count_init(c, max);
	}
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

	err = command_validate(cmd);
	if (err != SAM_ERR_OK) {
		command_free(cmd);
		return err;
	}

	for (File *file = vis->files; file; file = file->next) {
		if (file->internal)
			continue;
		sam_transcript_init(&file->transcript);
	}

	bool visual = vis->mode->visual;
	size_t primary_pos = vis->win ? view_cursor_get(vis->win->view) : EPOS;
	Filerange range = text_range_empty();
	sam_execute(vis, vis->win, cmd, NULL, &range);

	for (File *file = vis->files; file; file = file->next) {
		if (file->internal)
			continue;
		Transcript *t = &file->transcript;
		if (t->error != SAM_ERR_OK) {
			err = t->error;
			sam_transcript_free(t);
			continue;
		}
		vis_file_snapshot(vis, file);
		ptrdiff_t delta = 0;
		for (Change *c = t->changes; c; c = c->next) {
			c->range.start += delta;
			c->range.end += delta;
			if (c->type & TRANSCRIPT_DELETE) {
				text_delete_range(file->text, &c->range);
				delta -= text_range_size(&c->range);
				if (c->sel && c->type == TRANSCRIPT_DELETE) {
					if (visual)
						view_selections_dispose_force(c->sel);
					else
						view_cursors_to(c->sel, c->range.start);
				}
			}
			if (c->type & TRANSCRIPT_INSERT) {
				for (int i = 0; i < c->count; i++) {
					text_insert(file->text, c->range.start, c->data, c->len);
					delta += c->len;
				}
				Filerange r = text_range_new(c->range.start,
				                             c->range.start + c->len * c->count);
				if (c->sel) {
					if (visual) {
						view_selections_set(c->sel, &r);
						view_selections_anchor(c->sel, true);
					} else {
						if (memchr(c->data, '\n', c->len))
							view_cursors_to(c->sel, r.start);
						else
							view_cursors_to(c->sel, r.end);
					}
				} else if (visual) {
					Selection *sel = view_selections_new(c->win->view, r.start);
					if (sel) {
						view_selections_set(sel, &r);
						view_selections_anchor(sel, true);
					}
				}
			}
		}
		sam_transcript_free(&file->transcript);
		vis_file_snapshot(vis, file);
	}

	for (Win *win = vis->windows; win; win = win->next)
		view_selections_normalize(win->view);

	if (vis->win) {
		if (primary_pos != EPOS && view_selection_disposed(vis->win->view))
			view_cursor_to(vis->win->view, primary_pos);
		view_selections_primary_set(view_selections(vis->win->view));
		vis_jumplist_save(vis);
		bool completed = true;
		for (Selection *s = view_selections(vis->win->view); s; s = view_selections_next(s)) {
			if (view_selections_anchored(s)) {
				completed = false;
				break;
			}
		}
		vis_mode_switch(vis, completed ? VIS_MODE_NORMAL : VIS_MODE_VISUAL);
	}
	command_free(cmd);
	return err;
}

/* process text input, substitute register content for backreferences etc. */
Buffer text(Vis *vis, const char *text) {
	Buffer buf;
	buffer_init(&buf);
	for (size_t len = strcspn(text, "\\&"); *text; len = strcspn(++text, "\\&")) {
		buffer_append(&buf, text, len);
		text += len;
		enum VisRegister regid = VIS_REG_INVALID;
		switch (text[0]) {
		case '&':
			regid = VIS_REG_AMPERSAND;
			break;
		case '\\':
			if ('1' <= text[1] && text[1] <= '9') {
				regid = VIS_REG_1 + text[1] - '1';
				text++;
			} else if (text[1] == '\\' || text[1] == '&') {
				text++;
			}
			break;
		case '\0':
			goto out;
		}

		const char *data;
		size_t reglen = 0;
		if (regid != VIS_REG_INVALID) {
			data = register_get(vis, &vis->registers[regid], &reglen);
		} else {
			data = text;
			reglen = 1;
		}
		buffer_append(&buf, data, reglen);
	}
out:
	return buf;
}

static bool cmd_insert(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	Buffer buf = text(vis, argv[1]);
	size_t len = buffer_length(&buf);
	char *data = buffer_move(&buf);
	bool ret = sam_insert(win, sel, range->start, data, len, cmd->count.start);
	if (!ret)
		free(data);
	return ret;
}

static bool cmd_append(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	Buffer buf = text(vis, argv[1]);
	size_t len = buffer_length(&buf);
	char *data = buffer_move(&buf);
	bool ret = sam_insert(win, sel, range->end, data, len, cmd->count.start);
	if (!ret)
		free(data);
	return ret;
}

static bool cmd_change(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	Buffer buf = text(vis, argv[1]);
	size_t len = buffer_length(&buf);
	char *data = buffer_move(&buf);
	bool ret = sam_change(win, sel, range, data, len, cmd->count.start);
	if (!ret)
		free(data);
	return ret;
}

static bool cmd_delete(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	return win && sam_delete(win, sel, range);
}

static bool cmd_guard(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	bool match = false;
	RegexMatch captures[1];
	size_t len = text_range_size(range);
	if (!cmd->regex)
		match = true;
	else if (!text_search_range_forward(win->file->text, range->start, len, cmd->regex, 1, captures, 0))
		match = captures[0].start < range->end;
	if ((count_evaluate(cmd) && match) ^ (argv[0][0] == 'v'))
		return sam_execute(vis, win, cmd->cmd, sel, range);
	view_selections_dispose_force(sel);
	return true;
}

static int extract(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range, bool simulate) {
	bool ret = true;
	int count = 0;
	Text *txt = win->file->text;

	if (cmd->regex) {
		bool trailing_match = false;
		size_t start = range->start, end = range->end, last_start = EPOS;
		size_t nsub = 1 + text_regex_nsub(cmd->regex);
		if (nsub > MAX_REGEX_SUB)
			nsub = MAX_REGEX_SUB;
		RegexMatch match[MAX_REGEX_SUB];
		while (start < end || trailing_match) {
			trailing_match = false;
			char c;
			int flags = start > range->start &&
			            text_byte_get(txt, start - 1, &c) && c != '\n' ?
			            REG_NOTBOL : 0;
			bool found = !text_search_range_forward(txt, start, end - start,
			                                        cmd->regex, nsub, match,
			                                        flags);
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
				if (found) {
					for (size_t i = 0; i < nsub; i++) {
						Register *reg = &vis->registers[VIS_REG_AMPERSAND+i];
						register_put_range(vis, reg, txt, &match[i]);
					}
				}
				if (simulate)
					count++;
				else
					ret &= sam_execute(vis, win, cmd->cmd, NULL, &r);
				last_start = start;
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
			if (simulate)
				count++;
			else
				ret &= sam_execute(vis, win, cmd->cmd, NULL, &r);
			start = next;
		}
	}

	if (!simulate)
		view_selections_dispose_force(sel);
	return simulate ? count : ret;
}

static bool cmd_extract(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	int matches = 0;
	if (count_negative(cmd->cmd))
		matches = extract(vis, win, cmd, argv, sel, range, true);
	count_init(cmd->cmd, matches+1);
	return extract(vis, win, cmd, argv, sel, range, false);
}

static bool cmd_select(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	Filerange r = text_range_empty();
	if (!win)
		return sam_execute(vis, NULL, cmd->cmd, NULL, &r);
	bool ret = true;
	View *view = win->view;
	Text *txt = win->file->text;
	bool multiple_cursors = view_selections_count(view) > 1;
	Selection *primary = view_selections_primary_get(view);

	if (vis->mode->visual)
		count_init(cmd->cmd, view_selections_count(view)+1);

	for (Selection *s = view_selections(view), *next; s && ret; s = next) {
		next = view_selections_next(s);
		size_t pos = view_cursors_pos(s);
		if (vis->mode->visual) {
			r = view_selections_get(s);
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
			r = text_range_new(pos, pos);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_POS) {
			r = text_range_new(pos, pos);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_LINE) {
			r = text_object_line(txt, pos);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_AFTER) {
			size_t next_line = text_line_next(txt, pos);
			r = text_range_new(next_line, next_line);
		} else if (cmd->cmd->cmddef->flags & CMD_ADDRESS_ALL) {
			r = text_range_new(0, text_size(txt));
		} else if (!multiple_cursors && (cmd->cmd->cmddef->flags & CMD_ADDRESS_ALL_1CURSOR)) {
			r = text_range_new(0, text_size(txt));
		} else {
			r = text_range_new(pos, text_char_next(txt, pos));
		}
		if (!text_range_valid(&r))
			r = text_range_new(0, 0);
		ret &= sam_execute(vis, win, cmd->cmd, s, &r);
		if (cmd->cmd->cmddef->flags & CMD_ONCE)
			break;
	}

	if (vis->win && vis->win->view == view && primary != view_selections_primary_get(view))
		view_selections_primary_set(view_selections(view));
	return ret;
}

static bool cmd_print(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win || !text_range_valid(range))
		return false;
	View *view = win->view;
	if (!sel)
		sel = view_selections_new_force(view, range->start);
	if (!sel)
		return false;
	if (range->start != range->end) {
		view_selections_set(sel, range);
		view_selections_anchor(sel, true);
	} else {
		view_cursors_to(sel, range->start);
		view_selection_clear(sel);
	}
	return true;
}

static bool cmd_files(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	bool ret = true;
	for (Win *win = vis->windows; win; win = win->next) {
		if (win->file->internal)
			continue;
		bool match = !cmd->regex ||
		             (win->file->name && text_regex_match(cmd->regex, win->file->name, 0) == 0);
		if (match ^ (argv[0][0] == 'Y')) {
			Filerange def = text_range_new(0, 0);
			ret &= sam_execute(vis, win, cmd->cmd, NULL, &def);
		}
	}
	return ret;
}

static bool cmd_substitute(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	vis_info_show(vis, "Use :x/pattern/ c/replacement/ instead");
	return false;
}

/* cmd_write stores win->file's contents end emits pre/post events.
 * If the range r covers the whole file, it is updated to account for
 * potential file's text mutation by a FILE_SAVE_PRE callback.
 */
static bool cmd_write(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *r) {
	if (!win)
		return false;

	File *file = win->file;
	if (sam_transcript_error(&file->transcript, SAM_ERR_WRITE_CONFLICT))
		return false;

	Text *text = file->text;
	Filerange range_all = text_range_new(0, text_size(text));
	bool write_entire_file = text_range_equal(r, &range_all);

	const char *filename = argv[1];
	if (!filename)
		filename = file->name;
	if (!filename) {
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
		/* a pre-save hook may have changed the text; need to re-take the range */
		if (write_entire_file)
			*r = text_range_new(0, text_size(text));

		bool visual = vis->mode->visual;

		for (Selection *s = view_selections(win->view); s; s = view_selections_next(s)) {
			Filerange range = visual ? view_selections_get(s) : *r;
			ssize_t written = text_write_range(text, &range, file->fd);
			if (written == -1 || (size_t)written != text_range_size(&range)) {
				vis_info_show(vis, "Can not write to stdout");
				return false;
			}
			if (!visual)
				break;
		}

		/* make sure the file is marked as saved i.e. not modified */
		text_save(text, NULL);
		vis_event_emit(vis, VIS_EVENT_FILE_SAVE_POST, file, (char*)NULL);
		return true;
	}

	if (!argv[1] && cmd->flags != '!') {
		if (vis->mode->visual) {
			vis_info_show(vis, "WARNING: file will be reduced to active selection");
			return false;
		}
		if (!write_entire_file) {
			vis_info_show(vis, "WARNING: file will be reduced to provided range");
			return false;
		}
	}

	for (const char **name = argv[1] ? &argv[1] : (const char*[]){ filename, NULL }; *name; name++) {

		char *path = absolute_path(*name);
		if (!path)
			return false;

		struct stat meta;
		bool existing_file = !stat(path, &meta);
		bool same_file = existing_file && file->name &&
		                 file->stat.st_dev == meta.st_dev && file->stat.st_ino == meta.st_ino;

		if (cmd->flags != '!') {
			if (same_file && file->stat.st_mtime && file->stat.st_mtime < meta.st_mtime) {
				vis_info_show(vis, "WARNING: file has been changed since reading it");
				goto err;
			}
			if (existing_file && !same_file) {
				vis_info_show(vis, "WARNING: file exists");
				goto err;
			}
		}

		if (!vis_event_emit(vis, VIS_EVENT_FILE_SAVE_PRE, file, path) && cmd->flags != '!') {
			vis_info_show(vis, "Rejected write to `%s' by pre-save hook", path);
			goto err;
		}
		/* a pre-save hook may have changed the text; need to re-take the range */
		if (write_entire_file)
			*r = text_range_new(0, text_size(text));

		TextSave *ctx = text_save_begin(text, AT_FDCWD, path, file->save_method);
		if (!ctx) {
			const char *msg = errno ? strerror(errno) : "try changing `:set savemethod`";
			vis_info_show(vis, "Can't write `%s': %s", path, msg);
			goto err;
		}

		bool failure = false;
		bool visual = vis->mode->visual;

		for (Selection *s = view_selections(win->view); s; s = view_selections_next(s)) {
			Filerange range = visual ? view_selections_get(s) : *r;
			ssize_t written = text_save_write_range(ctx, &range);
			failure = (written == -1 || (size_t)written != text_range_size(&range));
			if (failure) {
				text_save_cancel(ctx);
				break;
			}

			if (!visual)
				break;
		}

		if (failure || !text_save_commit(ctx)) {
			vis_info_show(vis, "Can't write `%s': %s", path, strerror(errno));
			goto err;
		}

		if (!file->name) {
			file_name_set(file, path);
			same_file = true;
		}
		if (same_file || (!existing_file && strcmp(file->name, path) == 0))
			file->stat = text_stat(text);
		vis_event_emit(vis, VIS_EVENT_FILE_SAVE_POST, file, path);
		free(path);
		continue;

	err:
		free(path);
		return false;
	}
	return true;
}

static ssize_t read_buffer(void *context, char *data, size_t len) {
	buffer_append(context, data, len);
	return len;
}

static bool cmd_filter(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;

	Buffer bufout, buferr;
	buffer_init(&bufout);
	buffer_init(&buferr);

	int status = vis_pipe(vis, win->file, range, &argv[1], &bufout, read_buffer, &buferr, read_buffer);

	if (vis->interrupted) {
		vis_info_show(vis, "Command cancelled");
	} else if (status == 0) {
		size_t len = buffer_length(&bufout);
		char *data = buffer_move(&bufout);
		if (!sam_change(win, sel, range, data, len, 1))
			free(data);
	} else {
		vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));
	}

	buffer_release(&bufout);
	buffer_release(&buferr);

	return !vis->interrupted && status == 0;
}

static bool cmd_launch(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	Filerange invalid = text_range_new(sel ? view_cursors_pos(sel) : range->start, EPOS);
	return cmd_filter(vis, win, cmd, argv, sel, &invalid);
}

static bool cmd_pipein(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	Filerange filter_range = text_range_new(range->end, range->end);
	bool ret = cmd_filter(vis, win, cmd, argv, sel, &filter_range);
	if (ret)
		ret = sam_delete(win, NULL, range);
	return ret;
}

static bool cmd_pipeout(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	if (!win)
		return false;
	Buffer buferr;
	buffer_init(&buferr);

	int status = vis_pipe(vis, win->file, range, (const char*[]){ argv[1], NULL }, NULL, NULL, &buferr, read_buffer);

	if (vis->interrupted)
		vis_info_show(vis, "Command cancelled");
	else if (status != 0)
		vis_info_show(vis, "Command failed %s", buffer_content0(&buferr));

	buffer_release(&buferr);

	return !vis->interrupted && status == 0;
}

static bool cmd_cd(Vis *vis, Win *win, Command *cmd, const char *argv[], Selection *sel, Filerange *range) {
	const char *dir = argv[1];
	if (!dir)
		dir = getenv("HOME");
	return dir && chdir(dir) == 0;
}

#include "vis-cmds.c"
