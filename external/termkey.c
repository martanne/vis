////////////////////////////////////////////////////
// NOTE: this is an amalgamation of the original
// termkey made for the vis editor project
//
// - removes all functions unused by vis
// - namespaces everything to avoid conflicts when
//   #included
////////////////////////////////////////////////////

#ifndef TERMKEY_EXPORT
#define TERMKEY_EXPORT
#endif

#define UNIBI_EXPORT static
#include "unibilium.c"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef _WIN32
#include <poll.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#define HAVE_TERMIOS
#endif

#ifndef countof
#define countof(a) (sizeof(a) / sizeof(*(a)))
#endif

#define TERMKEY_VERSION_MAJOR 0
#define TERMKEY_VERSION_MINOR 22

#define TERMKEY_SYM_LIST \
	X(NONE,      "NONE") \
	X(BACKSPACE, "Backspace") \
	X(TAB,       "Tab") \
	X(ENTER,     "Enter") \
	X(ESCAPE,    "Escape") \
	X(SPACE,     "Space") \
	X(DEL,       "DEL") \
	X(UP,        "Up") \
	X(DOWN,      "Down") \
	X(LEFT,      "Left") \
	X(RIGHT,     "Right") \
	X(BEGIN,     "Begin") \
	X(FIND,      "Find") \
	X(INSERT,    "Insert") \
	X(DELETE,    "Delete") \
	X(SELECT,    "Select") \
	X(PAGEUP,    "PageUp") \
	X(PAGEDOWN,  "PageDown") \
	X(HOME,      "Home") \
	X(END,       "End") \
	X(CANCEL,    "Cancel") \
	X(CLEAR,     "Clear") \
	X(CLOSE,     "Close") \
	X(COMMAND,   "Command") \
	X(COPY,      "Copy") \
	X(EXIT,      "Exit") \
	X(HELP,      "Help") \
	X(MARK,      "Mark") \
	X(MESSAGE,   "Message") \
	X(MOVE,      "Move") \
	X(OPEN,      "Open") \
	X(OPTIONS,   "Options") \
	X(PRINT,     "Print") \
	X(REDO,      "Redo") \
	X(REFERENCE, "Reference") \
	X(REFRESH,   "Refresh") \
	X(REPLACE,   "Replace") \
	X(RESTART,   "Restart") \
	X(RESUME,    "Resume") \
	X(SAVE,      "Save") \
	X(SUSPEND,   "Suspend") \
	X(UNDO,      "Undo") \
	X(KP0,       "KP0") \
	X(KP1,       "KP1") \
	X(KP2,       "KP2") \
	X(KP3,       "KP3") \
	X(KP4,       "KP4") \
	X(KP5,       "KP5") \
	X(KP6,       "KP6") \
	X(KP7,       "KP7") \
	X(KP8,       "KP8") \
	X(KP9,       "KP9") \
	X(KPENTER,   "KPEnter") \
	X(KPPLUS,    "KPPlus") \
	X(KPMINUS,   "KPMinus") \
	X(KPMULT,    "KPMult") \
	X(KPDIV,     "KPDiv") \
	X(KPCOMMA,   "KPComma") \
	X(KPPERIOD,  "KPPeriod") \
	X(KPEQUALS,  "KPEquals") \

typedef enum {
	TERMKEY_SYM_UNKNOWN = -1,
	#define X(k, ...) TERMKEY_SYM_##k,
	TERMKEY_SYM_LIST
	#undef X
} TermKeySym;

#define X(_k, name) name,
static const char *termkey_keynames[] = {TERMKEY_SYM_LIST};
#undef X

#define X(_k, name) sizeof(name) - 1,
static uint8_t termkey_keyname_lengths[] = {TERMKEY_SYM_LIST};
#undef X

typedef enum {
	TERMKEY_TYPE_UNICODE,
	TERMKEY_TYPE_FUNCTION,
	TERMKEY_TYPE_KEYSYM,
	TERMKEY_TYPE_MOUSE,
	TERMKEY_TYPE_POSITION,
	TERMKEY_TYPE_MODEREPORT,
	TERMKEY_TYPE_DCS,
	TERMKEY_TYPE_OSC,
	/* add other recognised types here */

	TERMKEY_TYPE_UNKNOWN_CSI = -1
} TermKeyType;

typedef enum {
	TERMKEY_RES_NONE,
	TERMKEY_RES_KEY,
	TERMKEY_RES_EOF,
	TERMKEY_RES_AGAIN,
	TERMKEY_RES_ERROR
} TermKeyResult;

typedef enum {
	TERMKEY_MOUSE_UNKNOWN,
	TERMKEY_MOUSE_PRESS,
	TERMKEY_MOUSE_DRAG,
	TERMKEY_MOUSE_RELEASE
} TermKeyMouseEvent;

enum {
	TERMKEY_KEYMOD_SHIFT = 1 << 0,
	TERMKEY_KEYMOD_ALT   = 1 << 1,
	TERMKEY_KEYMOD_CTRL  = 1 << 2
};

typedef struct {
	TermKeyType type;
	union {
		long       codepoint; /* TERMKEY_TYPE_UNICODE */
		int        number;    /* TERMKEY_TYPE_FUNCTION */
		TermKeySym sym;       /* TERMKEY_TYPE_KEYSYM */
		char       mouse[4];  /* TERMKEY_TYPE_MOUSE */
		                      /* opaque. see termkey_interpret_mouse */
	} code;

	int modifiers;

	/* Any Unicode character can be UTF-8 encoded in no more than 4 bytes, plus
	 * terminating NUL */
	u8 utf8[5];
} TermKeyKey;

typedef enum {
	TERMKEY_FLAG_NOTERMIOS = 1 << 0, /* Do not make initial termios calls on construction */
} TermKeyStartFlags;

enum {
	TERMKEY_CANON_DELBS = 1 << 0  /* Del is converted to Backspace */
};

typedef enum {
	TERMKEY_FORMAT_LONGMOD     = 1 << 0, /* Shift-... instead of S-... */
	TERMKEY_FORMAT_CARETCTRL   = 1 << 1, /* ^X instead of C-X */
	TERMKEY_FORMAT_ALTISMETA   = 1 << 2, /* Meta- or M- instead of Alt- or A- */
	TERMKEY_FORMAT_WRAPBRACKET = 1 << 3, /* Wrap special keys in brackets like <Escape> */
	TERMKEY_FORMAT_SPACEMOD    = 1 << 4, /* M Foo instead of M-Foo */
	TERMKEY_FORMAT_LOWERMOD    = 1 << 5, /* meta or m instead of Meta or M */
	TERMKEY_FORMAT_LOWERSPACE  = 1 << 6, /* page down instead of PageDown */

	TERMKEY_FORMAT_MOUSE_POS   = 1 << 8  /* Include mouse position if relevant; @ col,line */
} TermKeyFormat;

/* Some useful combinations */

#define TERMKEY_FORMAT_VIM (TermKeyFormat)(TERMKEY_FORMAT_ALTISMETA|TERMKEY_FORMAT_WRAPBRACKET)
#define TERMKEY_FORMAT_URWID (TermKeyFormat)(TERMKEY_FORMAT_LONGMOD|TERMKEY_FORMAT_ALTISMETA| \
          TERMKEY_FORMAT_LOWERMOD|TERMKEY_FORMAT_SPACEMOD|TERMKEY_FORMAT_LOWERSPACE)

typedef struct termkey_trie_node termkey_trie_node;
typedef struct {
	unibi_term        *unibi;  /* only valid until first 'start' call */

	termkey_trie_node *root;

	char              *start_string;
	char              *stop_string;
} TermKeyTI;

typedef struct {
	int64_t  saved_string_id;
	char    *saved_string;
} TermKeyCSI;

typedef struct TermKey TermKey;
typedef struct TermKeyDriver TermKeyDriver;
struct TermKeyDriver {
	const char    *name;
	bool          (*init_driver)(TermKey *, TermKeyDriver *, const char *term);
	void          (*release_driver)(TermKeyDriver *);
	bool          (*start_driver)(TermKey *, TermKeyDriver *);
	bool          (*stop_driver)(TermKey *, TermKeyDriver *);
	TermKeyResult (*peekkey)(TermKey *, TermKeyDriver *, TermKeyKey *, int force, size_t *nbytes);

	union {
		TermKeyTI  ti;
		TermKeyCSI csi;
	} u;

	TermKeyDriver *next;
};

typedef struct {
	TermKeyType type;
	TermKeySym  sym;
	int         modifier_mask;
	int         modifier_set;
} TermKeyKeyInfo;

struct TermKey {
	int    fd;
	int    canonflags;
	unsigned char buffer[256];
	size_t buffstart; // First offset in buffer
	size_t buffcount; // NUMBER of entires valid in buffer
	size_t hightide;  /* Position beyond buffstart at which peekkey() should next start
	                   * normally 0, but see also termkey_interpret_csi */

	#ifdef HAVE_TERMIOS
	struct termios restore_termios;
	char restore_termios_valid;
	#endif

	char   is_closed;
	char   is_started;

	// There are 32 C0 codes
	TermKeyKeyInfo c0[32];

	TermKeyDriver *drivers;
};

#define CHARAT(i) (tk->buffer[tk->buffstart + (i)])

static inline void
termkey_key_get_linecol(const TermKeyKey *key, int *line, int *col)
{
	if (col)
		*col  = (unsigned char)key->code.mouse[1] | ((unsigned char)key->code.mouse[3] & 0x0f) << 8;

	if (line)
		*line = (unsigned char)key->code.mouse[2] | ((unsigned char)key->code.mouse[3] & 0x70) << 4;
}

static inline void
termkey_key_set_linecol(TermKeyKey *key, int line, int col)
{
	if(line > 0xfff)
		line = 0xfff;

	if(col > 0x7ff)
		col = 0x7ff;

	key->code.mouse[1] = (line & 0x0ff);
	key->code.mouse[2] = (col  & 0x0ff);
	key->code.mouse[3] = (line & 0xf00) >> 8 | (col & 0x300) >> 4;
}

static void
termkey_fill_utf8(TermKeyKey *key)
{
	u32 length = utf8_encode(key->utf8, key->code.codepoint);
	key->utf8[length] = 0;
}

TERMKEY_EXPORT void
termkey_canonicalise(TermKey *tk, TermKeyKey *key)
{
	int flags = tk->canonflags;

	if (key->type == TERMKEY_TYPE_KEYSYM && key->code.sym == TERMKEY_SYM_SPACE) {
		key->type           = TERMKEY_TYPE_UNICODE;
		key->code.codepoint = 0x20;
		termkey_fill_utf8(key);
	}

	if (flags & TERMKEY_CANON_DELBS)
		if (key->type == TERMKEY_TYPE_KEYSYM && key->code.sym == TERMKEY_SYM_DEL)
			key->code.sym = TERMKEY_SYM_BACKSPACE;
}

static void
termkey_emit_codepoint(TermKey *tk, long codepoint, TermKeyKey *key)
{
	if (codepoint == 0) {
		// ASCII NUL = Ctrl-Space
		key->type      = TERMKEY_TYPE_KEYSYM;
		key->code.sym  = TERMKEY_SYM_SPACE;
		key->modifiers = TERMKEY_KEYMOD_CTRL;
	} else if(codepoint < 0x20) {
		// C0 range
		key->code.codepoint = 0;
		key->modifiers      = 0;
		if (tk->c0[codepoint].sym != TERMKEY_SYM_UNKNOWN) {
			key->code.sym   = tk->c0[codepoint].sym;
			key->modifiers |= tk->c0[codepoint].modifier_set;
		}
		if (!key->code.sym) {
			key->type = TERMKEY_TYPE_UNICODE;
			/* Generically modified Unicode ought not report the SHIFT state, or else
			 * we get into complications trying to report Shift-; vs : and so on...
			 * In order to be able to represent Ctrl-Shift-A as CTRL modified
			 * unicode A, we need to call Ctrl-A simply 'a', lowercase */
			if (codepoint+0x40 >= 'A' && codepoint+0x40 <= 'Z')
				// it's a letter - use lowercase instead
				key->code.codepoint = codepoint + 0x60;
			else
				key->code.codepoint = codepoint + 0x40;
			key->modifiers = TERMKEY_KEYMOD_CTRL;
		} else {
			key->type = TERMKEY_TYPE_KEYSYM;
		}
	} else if (codepoint == 0x7f) {
		// ASCII DEL
		key->type      = TERMKEY_TYPE_KEYSYM;
		key->code.sym  = TERMKEY_SYM_DEL;
		key->modifiers = 0;
	} else if(codepoint >= 0x20 && codepoint < 0x80) {
		// ASCII lowbyte range
		key->type           = TERMKEY_TYPE_UNICODE;
		key->code.codepoint = codepoint;
		key->modifiers      = 0;
	} else if(codepoint >= 0x80 && codepoint < 0xa0) {
		// UTF-8 never starts with a C1 byte. So we can be sure of these
		key->type           = TERMKEY_TYPE_UNICODE;
		key->code.codepoint = codepoint - 0x40;
		key->modifiers      = TERMKEY_KEYMOD_CTRL|TERMKEY_KEYMOD_ALT;
	} else {
		// UTF-8 codepoint
		key->type           = TERMKEY_TYPE_UNICODE;
		key->code.codepoint = codepoint;
		key->modifiers      = 0;
	}

	termkey_canonicalise(tk, key);

	if (key->type == TERMKEY_TYPE_UNICODE)
		termkey_fill_utf8(key);
}

static TermKeyResult
termkey_peekkey_mouse(TermKey *tk, TermKeyKey *key, size_t *nbytep)
{
	if (tk->buffcount < 3)
		return TERMKEY_RES_AGAIN;
	key->type = TERMKEY_TYPE_MOUSE;
	key->code.mouse[0] = CHARAT(0) - 0x20;
	key->code.mouse[1] = CHARAT(1) - 0x20;
	key->code.mouse[2] = CHARAT(2) - 0x20;
	key->code.mouse[3] = 0;

	key->modifiers     = (key->code.mouse[0] & 0x1c) >> 2;
	key->code.mouse[0] &= ~0x1c;

	*nbytep = 3;
	return TERMKEY_RES_KEY;
}

/////////////////////////
// NOTE: Terminfo Driver
#define TERMKEY_TI_MAX_FUNCNAME 9

static struct {
	const char *funcname;
	TermKeyType type;
	TermKeySym sym;
	int mods;
} termkey_ti_funcs[] = {
  /* THIS LIST MUST REMAIN SORTED! */
  {"backspace", TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BACKSPACE, 0},
  {"begin",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN,     0},
  {"beg",       TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN,     0},
  {"btab",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB,       TERMKEY_KEYMOD_SHIFT},
  {"cancel",    TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_CANCEL,    0},
  {"clear",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_CLEAR,     0},
  {"close",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_CLOSE,     0},
  {"command",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_COMMAND,   0},
  {"copy",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_COPY,      0},
  {"dc",        TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    0},
  {"down",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,      0},
  {"end",       TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       0},
  {"enter",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_ENTER,     0},
  {"exit",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_EXIT,      0},
  {"find",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      0},
  {"help",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HELP,      0},
  {"home",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      0},
  {"ic",        TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    0},
  {"left",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,      0},
  {"mark",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_MARK,      0},
  {"message",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_MESSAGE,   0},
  {"move",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_MOVE,      0},
  {"next",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  0}, // Not quite, but it's the best we can do
  {"npage",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  0},
  {"open",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_OPEN,      0},
  {"options",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_OPTIONS,   0},
  {"ppage",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    0},
  {"previous",  TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    0}, // Not quite, but it's the best we can do
  {"print",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PRINT,     0},
  {"redo",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REDO,      0},
  {"reference", TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REFERENCE, 0},
  {"refresh",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REFRESH,   0},
  {"replace",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_REPLACE,   0},
  {"restart",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RESTART,   0},
  {"resume",    TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RESUME,    0},
  {"right",     TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT,     0},
  {"save",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SAVE,      0},
  {"select",    TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    0},
  {"suspend",   TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SUSPEND,   0},
  {"undo",      TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UNDO,      0},
  {"up",        TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,        0},
};

static enum unibi_string
termkey_unibi_lookup_str(const char *name)
{
	for (enum unibi_string ret = unibi_string_begin_ + 1; ret < unibi_string_end_; ret++)
		if (strcmp(unibi_name_str(ret), name) == 0)
			return ret;
	return -1;
}

static const char *
termkey_unibi_get_str_by_name(const unibi_term *ut, const char *name)
{
	enum unibi_string idx = termkey_unibi_lookup_str(name);
	const char *result = 0;
	if (idx != (enum unibi_string)-1)
		result = unibi_get_str(ut, idx);
	return result;
}

/* To be efficient at lookups, we store the byte sequence => keyinfo mapping
 * in a trie. This avoids a slow linear search through a flat list of
 * sequences. Because it is likely most nodes will be very sparse, we optimise
 * vector to store an extent map after the database is loaded.
 */

typedef enum {
	TermKeyTrieNodeKind_Key,
	TermKeyTrieNodeKind_Array,
} TermKeyTreeNodeKind;

struct termkey_trie_node {
	TermKeyTreeNodeKind kind;
};

typedef struct {
	TermKeyTreeNodeKind kind;
	TermKeyKeyInfo      key;
} termkey_trie_node_key;

typedef struct {
	TermKeyTreeNodeKind kind;
	unsigned char min, max; /* INCLUSIVE endpoints of the extent range */
	termkey_trie_node *arr[]; /* dynamic size at allocation time */
} termkey_trie_node_array;

static termkey_trie_node *
termkey_trie_new_node_arr(unsigned char min, unsigned char max)
{
	termkey_trie_node_array *result = calloc(1, sizeof(*result) + ((int)max - min + 1) * sizeof(result->arr[0]));
	if (result) {
		result->kind = TermKeyTrieNodeKind_Array;
		result->min  = min;
		result->max  = max;
	}
	return (termkey_trie_node *)result;
}

static termkey_trie_node *
termkey_trie_lookup_next(termkey_trie_node *n, unsigned char b)
{
	termkey_trie_node *result = 0;
	if (n->kind == TermKeyTrieNodeKind_Array) {
		termkey_trie_node_array *nar = (termkey_trie_node_array *)n;
		if (b >= nar->min && b <= nar->max)
			result = nar->arr[b - nar->min];
	}
	return result;
}

static int
termkey_trie_insert_seq(TermKeyTI *ti, const char *seq, termkey_trie_node *node)
{
	int pos = 0;
	termkey_trie_node *p = ti->root;

	// Unsigned because we'll be using it as an array subscript
	unsigned char b;
	while ((b = seq[pos])) {
		assert(p->kind == TermKeyTrieNodeKind_Array);
		termkey_trie_node *next = termkey_trie_lookup_next(p, b);
		if (!next)
			break;
		p = next;
		pos++;
	}

	while ((b = seq[pos])) {
		termkey_trie_node *next;
		if (seq[pos + 1]) {
			// Intermediate node
			next = termkey_trie_new_node_arr(0, 0xff);
		} else {
			// Final key node
			next = node;
		}

		if (!next) return 0;

		assert(p->kind == TermKeyTrieNodeKind_Array);
		termkey_trie_node_array *nar = (termkey_trie_node_array *)p;
		assert(b >= nar->min && b <= nar->max);
		nar->arr[b - nar->min] = next;

		p = next;
		pos++;
	}
	return 1;
}

static termkey_trie_node *
termkey_trie_new_node_key(TermKeyType type, TermKeySym sym, int modmask, int modset)
{
	termkey_trie_node_key *result = calloc(1, sizeof(*result));
	if (result) {
		result->kind = TermKeyTrieNodeKind_Key;
		result->key.type = type;
		result->key.sym  = sym;
		result->key.modifier_mask = modmask;
		result->key.modifier_set  = modset;
	}
	return (termkey_trie_node *)result;
}

static void
termkey_trie_free(termkey_trie_node *n)
{
	if (n->kind == TermKeyTrieNodeKind_Array) {
		termkey_trie_node_array *nar = (termkey_trie_node_array *)n;
		for (int i = nar->min; i <= nar->max; i++)
			if (nar->arr[i - nar->min])
				termkey_trie_free(nar->arr[i - nar->min]);
	}
	free(n);
}

static termkey_trie_node *
termkey_trie_compress(termkey_trie_node *n)
{
	if (n) {
		if (n->kind == TermKeyTrieNodeKind_Array) {
			termkey_trie_node_array *nar = (termkey_trie_node_array *)n;
			unsigned char min, max;
			// Find the real bounds
			for (min = 0; !nar->arr[min]; min++) {
				if(min == 255 && !nar->arr[min]) {
					free(nar);
					return termkey_trie_new_node_arr(1, 0);
				}
			}

			for (max = 0xff; !nar->arr[max]; max--) {}

			termkey_trie_node_array *new = (termkey_trie_node_array *)termkey_trie_new_node_arr(min, max);
			for (int i = min; i <= max; i++)
				new->arr[i - min] = termkey_trie_compress(nar->arr[i]);

			free(nar);
			n = (termkey_trie_node *)new;
		}
	}
	return n;
}

static bool
termkey_try_load_terminfo_key(TermKeyTI *ti, const char *name, TermKeyKeyInfo info)
{
	bool result = false;

	const char *value = 0;
	if (ti->unibi)
		value = termkey_unibi_get_str_by_name(ti->unibi, name);

	if (value && (value != (char *)-1) && value[0]) {
		termkey_trie_node *node = termkey_trie_new_node_key(info.type, info.sym,
		                                                    info.modifier_mask, info.modifier_set);
		termkey_trie_insert_seq(ti, value, node);
		result = true;
	}

	return result;
}

static int
termkey_load_terminfo(TermKeyTI *ti)
{
	ti->root = termkey_trie_new_node_arr(0, 0xff);
	if (!ti->root)
		return 0;

	/* First the regular key strings */
	for (int i = 0; i < (int)countof(termkey_ti_funcs); i++) {
		char name[TERMKEY_TI_MAX_FUNCNAME + 5 + 1];

		sprintf(name, "key_%s", termkey_ti_funcs[i].funcname);

		bool failed = !termkey_try_load_terminfo_key(ti, name, (TermKeyKeyInfo){
			.type          = termkey_ti_funcs[i].type,
			.sym           = termkey_ti_funcs[i].sym,
			.modifier_mask = termkey_ti_funcs[i].mods,
			.modifier_set  = termkey_ti_funcs[i].mods,
		});
		if (failed) continue;

		/* Maybe it has a shifted version */
		sprintf(name, "key_s%s", termkey_ti_funcs[i].funcname);
		termkey_try_load_terminfo_key(ti, name, (TermKeyKeyInfo){
			.type          = termkey_ti_funcs[i].type,
			.sym           = termkey_ti_funcs[i].sym,
			.modifier_mask = termkey_ti_funcs[i].mods | TERMKEY_KEYMOD_SHIFT,
			.modifier_set  = termkey_ti_funcs[i].mods | TERMKEY_KEYMOD_SHIFT,
		});
	}

	/* Now the F<digit> keys */
	for (int i = 1; i < 255; i++) {
		char name[9];
		sprintf(name, "key_f%d", i);
		bool failed = !termkey_try_load_terminfo_key(ti, name, (TermKeyKeyInfo){
			.type          = TERMKEY_TYPE_FUNCTION,
			.sym           = i,
			.modifier_mask = 0,
			.modifier_set  = 0,
		});
		if (failed) break;
	}

	/* Finally mouse mode */
	termkey_try_load_terminfo_key(ti, "key_mouse", (TermKeyKeyInfo){.type = TERMKEY_TYPE_MOUSE});

	/* Take copies of these terminfo strings, in case we build multiple termkey
	 * instances for multiple different termtypes, and it's different by the
	 * time we want to use it
	 */
	const char *keypad_xmit = ti->unibi ? unibi_get_str(ti->unibi, unibi_keypad_xmit) : 0;
	ti->start_string = keypad_xmit ? strdup(keypad_xmit) : 0;

	const char *keypad_local = ti->unibi ? unibi_get_str(ti->unibi, unibi_keypad_local) : 0;
    ti->stop_string = keypad_local ? strdup(keypad_local) : 0;

	if (ti->unibi) unibi_destroy(ti->unibi);

	ti->unibi = 0;
	ti->root  = termkey_trie_compress(ti->root);
	return 1;
}

static bool
termkey_ti_init_driver(TermKey *tk, TermKeyDriver *driver, const char *term)
{
	driver->u.ti.unibi = unibi_from_term(term);
	bool result = driver->u.ti.unibi != 0;
	return result;
}

static bool
termkey_ti_start_driver(TermKey *tk, TermKeyDriver *driver)
{
	TermKeyTI *ti = &driver->u.ti;
	if (!ti->root)
		termkey_load_terminfo(ti);

	char *start_string = ti->start_string;
	if (tk->fd == -1 || !start_string)
		return true;

	/* The terminfo database will contain keys in application cursor key mode.
	 * We may need to enable that mode
	 */

	/* There's no point trying to write() to a pipe */
	struct stat statbuf;
	if (fstat(tk->fd, &statbuf) == -1)
		return false;

	#ifndef _WIN32
	if (S_ISFIFO(statbuf.st_mode))
		return true;
	#endif

	// Can't call putp or tputs because they suck and don't give us fd control
	size_t len = strlen(start_string);
	while (len) {
		int64_t written = write(tk->fd, start_string, len);
		if (written == -1)
			return false;
		start_string += written;
		len          -= written;
	}
	return true;
}

static bool
termkey_ti_stop_driver(TermKey *tk, TermKeyDriver *driver)
{
	TermKeyTI *ti = &driver->u.ti;

	char *stop_string = ti->stop_string;
	if (tk->fd == -1 || !stop_string)
		return true;

	/* There's no point trying to write() to a pipe */
	struct stat statbuf;
	if (fstat(tk->fd, &statbuf) == -1)
		return false;

	#ifndef _WIN32
	if (S_ISFIFO(statbuf.st_mode))
		return true;
	#endif

	/* The terminfo database will contain keys in application cursor key mode.
	 * We may need to enable that mode
	 */

	// Can't call putp or tputs because they suck and don't give us fd control
	size_t len = strlen(stop_string);
	while(len) {
		int64_t written = write(tk->fd, stop_string, len);
		if (written == -1)
			return false;
		stop_string += written;
		len -= written;
	}
	return true;
}

static void
termkey_ti_release_driver(TermKeyDriver *driver)
{
	TermKeyTI *ti = &driver->u.ti;
	if (ti->root) termkey_trie_free(ti->root);
	free(ti->start_string);
	free(ti->stop_string);
	if (ti->unibi)
		unibi_destroy(ti->unibi);
	memset(&driver->u, 0, sizeof(driver->u));
}

static TermKeyResult
termkey_ti_peekkey(TermKey *tk, TermKeyDriver *driver, TermKeyKey *key, int force, size_t *nbytep)
{
	TermKeyTI *ti = &driver->u.ti;

	if (tk->buffcount == 0)
		return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

	termkey_trie_node *p = ti->root;

	unsigned int pos = 0;
	while (pos < tk->buffcount) {
		p = termkey_trie_lookup_next(p, CHARAT(pos));
		if (!p) break;
		pos++;

		if (p->kind != TermKeyTrieNodeKind_Key)
			continue;

		termkey_trie_node_key *nk = (termkey_trie_node_key *)p;
		if (nk->key.type == TERMKEY_TYPE_MOUSE) {
			tk->buffstart += pos;
			tk->buffcount -= pos;

			TermKeyResult mouse_result = termkey_peekkey_mouse(tk, key, nbytep);

			tk->buffstart -= pos;
			tk->buffcount += pos;

			if(mouse_result == TERMKEY_RES_KEY)
				*nbytep += pos;

			return mouse_result;
		}

		key->type      = nk->key.type;
		key->code.sym  = nk->key.sym;
		key->modifiers = nk->key.modifier_set;
		*nbytep = pos;
		return TERMKEY_RES_KEY;
	}

	// If p is not NULL then we hadn't walked off the end yet, so we have a partial match
	if (p && !force)
		return TERMKEY_RES_AGAIN;
	return TERMKEY_RES_NONE;
}

/////////////////////////
// NOTE: CSI Driver

// There are 64 codes 0x40 - 0x7F
static int            termkey_csi_keyinfo_initialised = 0;
static TermKeyKeyInfo termkey_csi_ss3s[64];
static char           termkey_csi_ss3_kpalts[64];

typedef TermKeyResult TermKeyCSIHandler(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args);
static TermKeyCSIHandler *termkey_csi_handlers[64];

/* This value must be increased if more CSI function keys are added */
static TermKeyKeyInfo termkey_csi_funcs[35];

/*
 * Handler for CSI/SS3 cmd keys
 */

static TermKeyResult
termkey_handle_csi_ss3_full(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args)
{
	if(args > 1 && arg[1] != -1)
		key->modifiers = arg[1] - 1;
	else
		key->modifiers = 0;

	key->type       =   termkey_csi_ss3s[cmd - 0x40].type;
	key->code.sym   =   termkey_csi_ss3s[cmd - 0x40].sym;
	key->modifiers &= ~(termkey_csi_ss3s[cmd - 0x40].modifier_mask);
	key->modifiers |=   termkey_csi_ss3s[cmd - 0x40].modifier_set;

	if (key->code.sym == TERMKEY_SYM_UNKNOWN)
		return TERMKEY_RES_NONE;
	return TERMKEY_RES_KEY;
}

static void
termkey_register_csi_ss3_full(TermKeyType type, TermKeySym sym, int modifier_set, int modifier_mask, unsigned char cmd)
{
	if (cmd >= 0x40 && cmd < 0x80) {
		termkey_csi_ss3s[cmd - 0x40].type          = type;
		termkey_csi_ss3s[cmd - 0x40].sym           = sym;
		termkey_csi_ss3s[cmd - 0x40].modifier_set  = modifier_set;
		termkey_csi_ss3s[cmd - 0x40].modifier_mask = modifier_mask;
		termkey_csi_handlers[cmd - 0x40] = termkey_handle_csi_ss3_full;
	}
}

static void
termkey_register_csi_ss3(TermKeyType type, TermKeySym sym, unsigned char cmd)
{
	termkey_register_csi_ss3_full(type, sym, 0, 0, cmd);
}

/*
 * Handler for SS3 keys with kpad alternate representations
 */
static void
termkey_register_ss3kpalt(TermKeyType type, TermKeySym sym, unsigned char cmd, char kpalt)
{
	if (cmd >= 0x40 && cmd < 0x80) {
		termkey_csi_ss3s[cmd - 0x40].type          = type;
		termkey_csi_ss3s[cmd - 0x40].sym           = sym;
		termkey_csi_ss3s[cmd - 0x40].modifier_set  = 0;
		termkey_csi_ss3s[cmd - 0x40].modifier_mask = 0;
		termkey_csi_ss3_kpalts[cmd - 0x40] = kpalt;
	}
}

/*
 * Handler for CSI number ~ function keys
 */
static TermKeyResult
termkey_handle_csifunc(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args)
{
	if(args > 1 && arg[1] != -1)
		key->modifiers = arg[1] - 1;
	else
		key->modifiers = 0;

	key->type = TERMKEY_TYPE_KEYSYM;

	if (arg[0] == 27) {
		int mod = key->modifiers;
		termkey_emit_codepoint(tk, arg[2], key);
		key->modifiers |= mod;
	} else if (arg[0] >= 0 && arg[0] < (long)countof(termkey_csi_funcs)) {
		key->type       =   termkey_csi_funcs[arg[0]].type;
		key->code.sym   =   termkey_csi_funcs[arg[0]].sym;
		key->modifiers &= ~(termkey_csi_funcs[arg[0]].modifier_mask);
		key->modifiers |=   termkey_csi_funcs[arg[0]].modifier_set;
	} else {
		key->code.sym = TERMKEY_SYM_UNKNOWN;
	}

	if (key->code.sym == TERMKEY_SYM_UNKNOWN)
		return TERMKEY_RES_NONE;
	return TERMKEY_RES_KEY;
}

static void
termkey_register_csifunc(TermKeyType type, TermKeySym sym, int number)
{
	if (number < (int)countof(termkey_csi_funcs)) {
		termkey_csi_funcs[number].type          = type;
		termkey_csi_funcs[number].sym           = sym;
		termkey_csi_funcs[number].modifier_set  = 0;
		termkey_csi_funcs[number].modifier_mask = 0;
		termkey_csi_handlers['~' - 0x40] = termkey_handle_csifunc;
	}
}

/*
 * Handler for CSI u extended Unicode keys
 */
static TermKeyResult
termkey_handle_csi_u(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args)
{
	TermKeyResult result = TERMKEY_RES_NONE;
	if (cmd == 'u') {
		if (args > 1 && arg[1] != -1)
			key->modifiers = arg[1] - 1;
		else
			key->modifiers = 0;

		int mod = key->modifiers;
		key->type = TERMKEY_TYPE_KEYSYM;
		termkey_emit_codepoint(tk, arg[0], key);
		key->modifiers |= mod;

		result =TERMKEY_RES_KEY;
	}
	return result;
}

/*
 * Handler for CSI M / CSI m mouse events in SGR and rxvt encodings
 * Note: This does not handle X10 encoding
 */
static TermKeyResult
termkey_handle_csi_m(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args)
{
	int initial = cmd >> 8;
	cmd &= 0xff;

	TermKeyResult result = TERMKEY_RES_NONE;
	if (cmd == 'M' || cmd == 'm') {
		if (!initial && args >= 3) { // rxvt protocol
			key->type = TERMKEY_TYPE_MOUSE;
			key->code.mouse[0] = arg[0];
			key->modifiers     = (key->code.mouse[0] & 0x1c) >> 2;
			key->code.mouse[0] &= ~0x1c;

			termkey_key_set_linecol(key, arg[1], arg[2]);
			result = TERMKEY_RES_KEY;
		} else if (initial == '<' && args >= 3) { // SGR protocol
			key->type = TERMKEY_TYPE_MOUSE;
			key->code.mouse[0] = arg[0];
			key->modifiers     = (key->code.mouse[0] & 0x1c) >> 2;
			key->code.mouse[0] &= ~0x1c;

			termkey_key_set_linecol(key, arg[1], arg[2]);

			if (cmd == 'm') // release
				key->code.mouse[3] |= 0x80;

			result = TERMKEY_RES_KEY;
		}
	}
	return result;
}

TERMKEY_EXPORT TermKeyResult
termkey_interpret_mouse(TermKey *tk, const TermKeyKey *key, TermKeyMouseEvent *event, int *button, int *line, int *col)
{
	if (key->type != TERMKEY_TYPE_MOUSE)
		return TERMKEY_RES_NONE;

	if (button)
		*button = 0;

	termkey_key_get_linecol(key, line, col);

	if (!event)
		return TERMKEY_RES_KEY;

	int btn  = 0;
	int code = key->code.mouse[0];
	int drag = code & 0x20;
	code &= ~0x3c;

	switch (code) {
	case 0:
	case 1:
	case 2:
	{
		*event = drag ? TERMKEY_MOUSE_DRAG : TERMKEY_MOUSE_PRESS;
		btn = code + 1;
	}break;

	case 3:{
		*event = TERMKEY_MOUSE_RELEASE;
		// no button hint
	}break;

	case 64:
	case 65:
    {
		*event = drag ? TERMKEY_MOUSE_DRAG : TERMKEY_MOUSE_PRESS;
		btn = code + 4 - 64;
	}break;

	default:{
		*event = TERMKEY_MOUSE_UNKNOWN;
	}break;
	}

	if (button)
		*button = btn;

	if (key->code.mouse[3] & 0x80)
		*event = TERMKEY_MOUSE_RELEASE;

	return TERMKEY_RES_KEY;
}

/*
 * Handler for CSI ? R position reports
 * A plain CSI R with no arguments is probably actually <F3>
 */
static TermKeyResult
termkey_handle_csi_R(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args)
{
	if (cmd == ('R'|'?'<<8)) {
		if(args < 2)
			return TERMKEY_RES_NONE;
		key->type = TERMKEY_TYPE_POSITION;
		termkey_key_set_linecol(key, arg[1], arg[0]);
		return TERMKEY_RES_KEY;
	}
	return termkey_handle_csi_ss3_full(tk, key, cmd, arg, args);
}

/*
 * Handler for CSI $y mode status reports
 */
static TermKeyResult
termkey_handle_csi_y(TermKey *tk, TermKeyKey *key, int cmd, long *arg, int args)
{
	TermKeyResult result = TERMKEY_RES_NONE;
	if (cmd == ('y'|'$'<<16) || cmd == ('y'|'$'<<16 | '?'<<8)) {
		if (args >= 2) {
			key->type = TERMKEY_TYPE_MODEREPORT;
			key->code.mouse[0] = (cmd >> 8);
			key->code.mouse[1] = arg[0] >> 8;
			key->code.mouse[2] = arg[0] & 0xff;
			key->code.mouse[3] = arg[1];
			result = TERMKEY_RES_KEY;
		}
	}
	return result;
}

TERMKEY_EXPORT TermKeyResult
termkey_interpret_modereport(TermKey *tk, const TermKeyKey *key, int *initial, int *mode, int *value)
{
	if (key->type != TERMKEY_TYPE_MODEREPORT)
		return TERMKEY_RES_NONE;

	if (initial)
		*initial = key->code.mouse[0];

	if (mode)
		*mode = (key->code.mouse[1] << 8) | key->code.mouse[2];

	if (value)
		*value = key->code.mouse[3];

	return TERMKEY_RES_KEY;
}

static TermKeyResult
termkey_parse_csi(TermKey *tk, size_t introlen, size_t *csi_len, long args[], size_t *nargs, unsigned long *commandp)
{
	size_t csi_end = introlen;

	while (csi_end < tk->buffcount) {
		if (CHARAT(csi_end) >= 0x40 && CHARAT(csi_end) < 0x80)
			break;
		csi_end++;
	}

	if (csi_end >= tk->buffcount)
		return TERMKEY_RES_AGAIN;

	unsigned char cmd = CHARAT(csi_end);
	*commandp = cmd;

	char   present = 0;
	int    argi    = 0;
	size_t p       = introlen;

	// See if there is an initial byte
	if (CHARAT(p) >= '<' && CHARAT(p) <= '?') {
		*commandp |= (CHARAT(p) << 8);
		p++;
	}

	// Now attempt to parse out up number;number;... separated values
	while (p < csi_end) {
		unsigned char c = CHARAT(p);

		if (c >= '0' && c <= '9') {
			if (!present) {
				args[argi] = c - '0';
				present = 1;
			} else {
				args[argi] = (args[argi] * 10) + c - '0';
			}
		} else if(c == ';') {
			if (!present)
				args[argi] = -1;
			present = 0;
			argi++;

			if (argi > 16)
				break;
		} else if(c >= 0x20 && c <= 0x2f) {
			*commandp |= c << 16;
			break;
		}
		p++;
	}

	if (present)
		argi++;

	if (csi_len) *csi_len = csi_end + 1;
	*nargs = argi;
	return TERMKEY_RES_KEY;
}

TERMKEY_EXPORT TermKeyResult
termkey_interpret_csi(TermKey *tk, const TermKeyKey *key, long args[], size_t *nargs, unsigned long *cmd)
{
	TermKeyResult result = TERMKEY_RES_NONE;
	if (tk->hightide != 0 && key->type == TERMKEY_TYPE_UNKNOWN_CSI)
		result = termkey_parse_csi(tk, 0, 0, args, nargs, cmd);
	return result;
}

static void
termkey_register_keys(void)
{
	for (int i = 0; i < 64; i++) {
		termkey_csi_ss3s[i].sym = TERMKEY_SYM_UNKNOWN;
		termkey_csi_ss3s[i].sym = TERMKEY_SYM_UNKNOWN;
		termkey_csi_ss3_kpalts[i] = 0;
	}

	for (int i = 0; i < (int)countof(termkey_csi_funcs); i++)
		termkey_csi_funcs[i].sym = TERMKEY_SYM_UNKNOWN;

	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_UP,    'A');
	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DOWN,  'B');
	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_RIGHT, 'C');
	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_LEFT,  'D');
	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_BEGIN, 'E');
	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,   'F');
	termkey_register_csi_ss3(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,  'H');
	termkey_register_csi_ss3(TERMKEY_TYPE_FUNCTION, 1, 'P');
	termkey_register_csi_ss3(TERMKEY_TYPE_FUNCTION, 2, 'Q');
	termkey_register_csi_ss3(TERMKEY_TYPE_FUNCTION, 3, 'R');
	termkey_register_csi_ss3(TERMKEY_TYPE_FUNCTION, 4, 'S');

	termkey_register_csi_ss3_full(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_TAB, TERMKEY_KEYMOD_SHIFT, TERMKEY_KEYMOD_SHIFT, 'Z');

	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPENTER,  'M', 0);
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPEQUALS, 'X', '=');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMULT,   'j', '*');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPLUS,   'k', '+');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPCOMMA,  'l', ',');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPMINUS,  'm', '-');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPPERIOD, 'n', '.');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KPDIV,    'o', '/');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP0,      'p', '0');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP1,      'q', '1');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP2,      'r', '2');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP3,      's', '3');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP4,      't', '4');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP5,      'u', '5');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP6,      'v', '6');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP7,      'w', '7');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP8,      'x', '8');
	termkey_register_ss3kpalt(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_KP9,      'y', '9');

	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_FIND,      1);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_INSERT,    2);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_DELETE,    3);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_SELECT,    4);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEUP,    5);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_PAGEDOWN,  6);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_HOME,      7);
	termkey_register_csifunc(TERMKEY_TYPE_KEYSYM, TERMKEY_SYM_END,       8);

	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 1,  11);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 2,  12);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 3,  13);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 4,  14);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 5,  15);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 6,  17);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 7,  18);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 8,  19);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 9,  20);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 10, 21);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 11, 23);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 12, 24);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 13, 25);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 14, 26);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 15, 28);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 16, 29);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 17, 31);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 18, 32);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 19, 33);
	termkey_register_csifunc(TERMKEY_TYPE_FUNCTION, 20, 34);

	termkey_csi_handlers['u' - 0x40] = termkey_handle_csi_u;
	termkey_csi_handlers['M' - 0x40] = termkey_handle_csi_m;
	termkey_csi_handlers['m' - 0x40] = termkey_handle_csi_m;
	termkey_csi_handlers['R' - 0x40] = termkey_handle_csi_R;
	termkey_csi_handlers['y' - 0x40] = termkey_handle_csi_y;

	termkey_csi_keyinfo_initialised = 1;
}

static bool
termkey_csi_init_driver(TermKey *tk, TermKeyDriver *driver, const char *term)
{
	if (!termkey_csi_keyinfo_initialised)
		termkey_register_keys();
	return true;
}

static void
termkey_csi_release_driver(TermKeyDriver *driver)
{
	free(driver->u.csi.saved_string);
	memset(&driver->u, 0, sizeof(driver->u));
}

static TermKeyResult
termkey_peekkey_csi(TermKey *tk, TermKeyCSI *csi, size_t introlen, TermKeyKey *key, int force, size_t *nbytep)
{
	size_t csi_len;
	size_t args = 16;
	long arg[16];
	unsigned long cmd;

	TermKeyResult result = termkey_parse_csi(tk, introlen, &csi_len, arg, &args, &cmd);
	if (result == TERMKEY_RES_AGAIN) {
		if(!force)
			return TERMKEY_RES_AGAIN;

		termkey_emit_codepoint(tk, '[', key);
		key->modifiers |= TERMKEY_KEYMOD_ALT;
		*nbytep = introlen;
		return TERMKEY_RES_KEY;
	}

	if (cmd == 'M' && args < 3) { // Mouse in X10 encoding consumes the next 3 bytes also
		tk->buffstart += csi_len;
		tk->buffcount -= csi_len;

		result = termkey_peekkey_mouse(tk, key, nbytep);

		tk->buffstart -= csi_len;
		tk->buffcount += csi_len;

		if (result == TERMKEY_RES_KEY)
			*nbytep += csi_len;

		return result;
	}

	result = TERMKEY_RES_NONE;

	// We know from the logic above that cmd must be >= 0x40 and < 0x80
	if (termkey_csi_handlers[(cmd & 0xff) - 0x40])
		result = termkey_csi_handlers[(cmd & 0xff) - 0x40](tk, key, cmd, arg, args);

	if (result == TERMKEY_RES_NONE) {
		key->type        = TERMKEY_TYPE_UNKNOWN_CSI;
		key->code.number = cmd;
		key->modifiers   = 0;

		tk->hightide = csi_len - introlen;
		*nbytep = introlen; // Do not yet eat the data bytes
		return TERMKEY_RES_KEY;
	}

	*nbytep = csi_len;
	return result;
}

static TermKeyResult
termkey_peekkey_ss3(TermKey *tk, TermKeyCSI *csi, size_t introlen, TermKeyKey *key, int force, size_t *nbytep)
{
	if (tk->buffcount < introlen + 1) {
		if (!force)
			return TERMKEY_RES_AGAIN;

		termkey_emit_codepoint(tk, 'O', key);
		key->modifiers |= TERMKEY_KEYMOD_ALT;
		*nbytep = tk->buffcount;
		return TERMKEY_RES_KEY;
	}

	unsigned char cmd = CHARAT(introlen);

	if (cmd < 0x40 || cmd >= 0x80)
		return TERMKEY_RES_NONE;

	key->type      = termkey_csi_ss3s[cmd - 0x40].type;
	key->code.sym  = termkey_csi_ss3s[cmd - 0x40].sym;
	key->modifiers = termkey_csi_ss3s[cmd - 0x40].modifier_set;

	if (key->code.sym == TERMKEY_SYM_UNKNOWN) {
		key->type      = termkey_csi_ss3s[cmd - 0x40].type;
		key->code.sym  = termkey_csi_ss3s[cmd - 0x40].sym;
		key->modifiers = termkey_csi_ss3s[cmd - 0x40].modifier_set;
	}

	if (key->code.sym == TERMKEY_SYM_UNKNOWN)
		return TERMKEY_RES_NONE;

	*nbytep = introlen + 1;
	return TERMKEY_RES_KEY;
}

static TermKeyResult
termkey_peekkey_ctrlstring(TermKey *tk, TermKeyCSI *csi, size_t introlen, TermKeyKey *key, int force, size_t *nbytep)
{
	size_t str_end = introlen;
	while (str_end < tk->buffcount) {
		if (CHARAT(str_end) == 0x9c) // ST
			break;
		if (CHARAT(str_end) == 0x1b && (str_end + 1) < tk->buffcount && CHARAT(str_end + 1) == 0x5c) // ESC-prefixed ST
			break;
		str_end++;
	}

	if (str_end >= tk->buffcount)
		return TERMKEY_RES_AGAIN;

	*nbytep = str_end + 1;
	if (CHARAT(str_end) == 0x1b)
		(*nbytep)++;

	free(csi->saved_string);

	size_t len = str_end - introlen;

	csi->saved_string_id++;
	csi->saved_string = strndup((char *)tk->buffer + tk->buffstart + introlen, len);

	key->type        = (CHARAT(introlen-1) & 0x1f) == 0x10 ? TERMKEY_TYPE_DCS : TERMKEY_TYPE_OSC;
	key->code.number = csi->saved_string_id;
	key->modifiers   = 0;
	return TERMKEY_RES_KEY;
}

static TermKeyResult
termkey_csi_peekkey(TermKey *tk, TermKeyDriver *driver, TermKeyKey *key, int force, size_t *nbytep)
{
	if (tk->buffcount == 0)
		return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

	TermKeyCSI *csi = &driver->u.csi;

	TermKeyResult result = TERMKEY_RES_NONE;
	switch (CHARAT(0)) {
	case 0x1b:{
		if (tk->buffcount >= 2) switch(CHARAT(1)) {
		// ESC-prefixed SS3
		case 0x4f:{result = termkey_peekkey_ss3(tk, csi, 2, key, force, nbytep);}break;

		case 0x50: // ESC-prefixed DCS
		case 0x5d: // ESC-prefixed OSC
		{
			result = termkey_peekkey_ctrlstring(tk, csi, 2, key, force, nbytep);
		}break;

		// ESC-prefixed CSI
		case 0x5b:{result = termkey_peekkey_csi(tk, csi, 2, key, force, nbytep);}break;
		}
	}break;

	// SS3
	case 0x8f:{result = termkey_peekkey_ss3(tk, csi, 1, key, force, nbytep);}break;

	case 0x90: // DCS
	case 0x9d: // OSC
	{
		result = termkey_peekkey_ctrlstring(tk, csi, 1, key, force, nbytep);
	}break;

	// CSI
	case 0x9b:{result = termkey_peekkey_csi(tk, csi, 1, key, force, nbytep);}break;
	}

	return result;
}

/////////////////////////
// NOTE: TermKey Implementation

#ifdef _MSC_VER
#define strcaseeq(a,b) (_stricmp(a,b) == 0)
#else
#define strcaseeq(a,b) (strcasecmp(a,b) == 0)
#endif

static TermKeyDriver termkey_drivers[] = {
	{
		.name           = "terminfo",
		.init_driver    = termkey_ti_init_driver,
		.release_driver = termkey_ti_release_driver,
		.start_driver   = termkey_ti_start_driver,
		.stop_driver    = termkey_ti_stop_driver,
		.peekkey        = termkey_ti_peekkey,
	},
	{
		.name           = "CSI",
		.init_driver    = termkey_csi_init_driver,
		.release_driver = termkey_csi_release_driver,
		.peekkey        = termkey_csi_peekkey,
	},
};

// Mouse event names
static const char *termkey_evnames[] = {"Unknown", "Press", "Drag", "Release"};

/* Similar to snprintf(str, size, "%s", src) except it turns CamelCase into
 * space separated values
 */
static int
termkey_snprint_cameltospaces(char *str, size_t size, const char *src)
{
	int prev_lower = 0;
	size_t l = 0;
	while (*src && l < size - 1) {
		if (isupper(*src) && prev_lower) {
			if (str)
				str[l++] = ' ';
		if (l >= size - 1)
			break;
		}
		prev_lower = islower(*src);
		str[l++]   = tolower(*src++);
	}
	str[l] = 0;
	/* For consistency with snprintf, return the number of bytes that would have
	 * been written, excluding '\0' */
	while (*src) {
		if (isupper(*src) && prev_lower)
			l++;
		prev_lower = islower(*src);
		src++; l++;
	}
	return l;
}

/* Similar to strcmp(str, strcamel, n) except that:
 *    it compares CamelCase in strcamel with space separated values in str;
 *    it takes char**s and updates them
 * n counts bytes of strcamel, not str
 */
static int
termkey_strpncmp_camel(const char **strp, const char **strcamelp, size_t n)
{
	const char *str = *strp, *strcamel = *strcamelp;
	int prev_lower = 0;

	for (;(*str || *strcamel) && n; n--) {
		char b = tolower(*strcamel);
		if (isupper(*strcamel) && prev_lower) {
			if(*str != ' ')
				break;
			str++;
			if (*str != b)
				break;
		} else if (*str != b) {
			break;
		}
		prev_lower = islower(*strcamel);
		str++;
		strcamel++;
	}

	*strp      = str;
	*strcamelp = strcamel;
	return *str - *strcamel;
}

static void
termkey_register_c0(TermKey *tk, TermKeySym sym, unsigned char ctrl)
{
	assert(ctrl < 0x20);
	tk->c0[ctrl].sym           = sym;
	tk->c0[ctrl].modifier_set  = 0;
	tk->c0[ctrl].modifier_mask = 0;
}

TERMKEY_EXPORT bool
termkey_start(TermKey *tk, TermKeyStartFlags start_flags)
{
	if (tk->is_started)
		return true;

	#ifdef HAVE_TERMIOS
	if (tk->fd != -1 && !(start_flags & TERMKEY_FLAG_NOTERMIOS)) {
		struct termios termios;
		if (tcgetattr(tk->fd, &termios) == 0) {
			tk->restore_termios       = termios;
			tk->restore_termios_valid = 1;

			termios.c_iflag &= ~(IXON|INLCR|ICRNL);
			termios.c_lflag &= ~(ICANON|ECHO
			#ifdef IEXTEN
			| IEXTEN
			#endif
			);
			termios.c_cc[VMIN] = 1;
			termios.c_cc[VTIME] = 0;

			/* Disable Ctrl-\==VQUIT and Ctrl-D==VSUSP but leave Ctrl-C as SIGINT */
			termios.c_cc[VQUIT] = _POSIX_VDISABLE;
			termios.c_cc[VSUSP] = _POSIX_VDISABLE;
			/* Some OSes have Ctrl-Y==VDSUSP */
			#ifdef VDSUSP
			termios.c_cc[VDSUSP] = _POSIX_VDISABLE;
			#endif
			tcsetattr(tk->fd, TCSANOW, &termios);
		}
	}
	#endif

	for (TermKeyDriver *p = tk->drivers; p; p = p->next)
		if (p->start_driver && !p->start_driver(tk, p))
			return false;

	tk->is_started = 1;
	return true;
}

TERMKEY_EXPORT bool
termkey_init_from_fd(TermKey *tk, int fd, TermKeyStartFlags start_flags, char *term)
{
	tk->fd = fd;

	termkey_register_c0(tk, TERMKEY_SYM_TAB,    0x09);
	termkey_register_c0(tk, TERMKEY_SYM_ENTER,  0x0d);
	termkey_register_c0(tk, TERMKEY_SYM_ESCAPE, 0x1b);

	TermKeyDriver *tail = 0;
	for (int i = 0; i < (int)countof(termkey_drivers); i++) {
		if (!termkey_drivers[i].init_driver(tk, termkey_drivers + i, term))
			continue;

		if (!tail) tk->drivers = termkey_drivers + i;
		else       tail->next  = termkey_drivers + i;
		tail = termkey_drivers + i;
	}

	bool result = termkey_start(tk, start_flags);
	return result;
}

TERMKEY_EXPORT void
termkey_release(TermKey *tk)
{
	for (TermKeyDriver *p = tk->drivers; p;) {
		p->release_driver(p);
		TermKeyDriver *next = p->next;
		p->next = 0;
		p       = next;
	}
}

TERMKEY_EXPORT int
termkey_stop(TermKey *tk)
{
	if (tk->is_started) {
		for (TermKeyDriver *p = tk->drivers; p; p = p->next)
			if (p->stop_driver)
				p->stop_driver(tk, p);

		#ifdef HAVE_TERMIOS
		if (tk->restore_termios_valid)
			tcsetattr(tk->fd, TCSANOW, &tk->restore_termios);
		#endif
		tk->is_started = 0;
	}
	return 1;
}

TERMKEY_EXPORT void
termkey_destroy(TermKey *tk)
{
	termkey_stop(tk);
	termkey_release(tk);
	memset(tk, 0, sizeof(*tk));
	tk->fd = -1;
}

TERMKEY_EXPORT void
termkey_set_canonflags(TermKey *tk, int flags)
{
	tk->canonflags = flags;
}

static void
termkey_eat_bytes(TermKey *tk, size_t count)
{
	if (count >= tk->buffcount) {
		tk->buffstart = 0;
		tk->buffcount = 0;
		return;
	}
	tk->buffstart += count;
	tk->buffcount -= count;
}

#define UTF8_INVALID 0xFFFD
static TermKeyResult
termkey_parse_utf8(const unsigned char *bytes, size_t len, long *cp, size_t *nbytep)
{
	unsigned int nbytes;

	unsigned char b0 = bytes[0];

	if (b0 < 0x80) {
		// Single byte ASCII
		*cp = b0;
		*nbytep = 1;
		return TERMKEY_RES_KEY;
	} else if (b0 < 0xc0) {
		// Starts with a continuation byte - that's not right
		*cp = UTF8_INVALID;
		*nbytep = 1;
		return TERMKEY_RES_KEY;
	} else if(b0 < 0xe0) {
		nbytes = 2;
		*cp = b0 & 0x1f;
	} else if(b0 < 0xf0) {
		nbytes = 3;
		*cp = b0 & 0x0f;
	} else if(b0 < 0xf8) {
		nbytes = 4;
		*cp = b0 & 0x07;
	} else {
		*cp = UTF8_INVALID;
		*nbytep = 1;
		return TERMKEY_RES_KEY;
	}

	for (unsigned int b = 1; b < nbytes; b++) {
		unsigned char cb;

		if (b >= len)
			return TERMKEY_RES_AGAIN;

		cb = bytes[b];
		if (cb < 0x80 || cb >= 0xc0) {
			*cp = UTF8_INVALID;
			*nbytep = b;
			return TERMKEY_RES_KEY;
		}
		*cp <<= 6;
		*cp |= cb & 0x3f;
	}

	// Check for overlong sequences
	if (nbytes > utf8_sequence_length(*cp))
		*cp = UTF8_INVALID;

	// Check for UTF-16 surrogates or invalid *cps
	if ((*cp >= 0xD800 && *cp <= 0xDFFF) || *cp == 0xFFFE || *cp == 0xFFFF)
		*cp = UTF8_INVALID;

	*nbytep = nbytes;
	return TERMKEY_RES_KEY;
}

// TODO(rnp): cleanup: dumb indirect recursion
static TermKeyResult termkey_peekkey(TermKey *, TermKeyKey *, int, size_t *);

static TermKeyResult
termkey_peekkey_simple(TermKey *tk, TermKeyKey *key, int force, size_t *nbytep)
{
	if (tk->buffcount == 0)
		return tk->is_closed ? TERMKEY_RES_EOF : TERMKEY_RES_NONE;

	unsigned char b0 = CHARAT(0);

	if (b0 == 0x1b) {
		// Escape-prefixed value? Might therefore be Alt+key
		if (tk->buffcount == 1) {
			// This might be an <Esc> press, or it may want to be part of a longer sequence
			if (!force)
				return TERMKEY_RES_AGAIN;

			termkey_emit_codepoint(tk, b0, key);
			*nbytep = 1;
			return TERMKEY_RES_KEY;
		}

		// Try another key there
		tk->buffstart++;
		tk->buffcount--;

		// Run the full driver
		TermKeyResult metakey_result = termkey_peekkey(tk, key, force, nbytep);
		tk->buffstart--;
		tk->buffcount++;

		switch(metakey_result) {
		case TERMKEY_RES_KEY:{
			key->modifiers |= TERMKEY_KEYMOD_ALT;
			(*nbytep)++;
		}break;

		case TERMKEY_RES_NONE:
		case TERMKEY_RES_EOF:
		case TERMKEY_RES_AGAIN:
		case TERMKEY_RES_ERROR:
		{}break;
		}

		return metakey_result;
	} else if(b0 < 0xa0) {
		// Single byte C0, G0 or C1 - C1 is never UTF-8 initial byte
		termkey_emit_codepoint(tk, b0, key);
		*nbytep = 1;
		return TERMKEY_RES_KEY;
	}
	// Some UTF-8
	long codepoint;
	TermKeyResult result = termkey_parse_utf8(tk->buffer + tk->buffstart, tk->buffcount, &codepoint, nbytep);
	if (result == TERMKEY_RES_AGAIN && force) {
		/* There weren't enough bytes for a complete UTF-8 sequence but caller
		 * demands an answer. About the best thing we can do here is eat as many
		 * bytes as we have, and emit a UTF8_INVALID. If the remaining bytes
		 * arrive later, they'll be invalid too. */
		codepoint = UTF8_INVALID;
		*nbytep   = tk->buffcount;
		result    = TERMKEY_RES_KEY;
	}

	key->type = TERMKEY_TYPE_UNICODE;
	key->modifiers = 0;
	termkey_emit_codepoint(tk, codepoint, key);
	return result;
}

static TermKeyResult
termkey_peekkey(TermKey *tk, TermKeyKey *key, int force, size_t *nbytep)
{
	int again = 0;

	if (!tk->is_started) {
		errno = EINVAL;
		return TERMKEY_RES_ERROR;
	}

	if (tk->hightide) {
		tk->buffstart += tk->hightide;
		tk->buffcount -= tk->hightide;
		tk->hightide   = 0;
	}

	TermKeyResult result;
	for (TermKeyDriver *p = tk->drivers; p; p = p->next) {
		result = p->peekkey(tk, p, key, force, nbytep);

		switch (result) {
		case TERMKEY_RES_NONE:{}break;

		case TERMKEY_RES_EOF:
		case TERMKEY_RES_ERROR:
		{
			return result;
		}break;

		case TERMKEY_RES_AGAIN:{
			if (!force)
				again = 1;
		}break;

		case TERMKEY_RES_KEY:{ // Slide the data down to stop it running away
			size_t halfsize = sizeof(tk->buffer) / 2;
			if(tk->buffstart > halfsize) {
				memcpy(tk->buffer, tk->buffer + halfsize, halfsize);
				tk->buffstart -= halfsize;
			}
			return result;
		}break;
		}
	}

	if (again) return TERMKEY_RES_AGAIN;

	return termkey_peekkey_simple(tk, key, force, nbytep);
}

TERMKEY_EXPORT TermKeyResult
termkey_getkey(TermKey *tk, TermKeyKey *key)
{
	size_t nbytes = 0;
	TermKeyResult result = termkey_peekkey(tk, key, 0, &nbytes);

	if (result == TERMKEY_RES_KEY)
		termkey_eat_bytes(tk, nbytes);

	if (result == TERMKEY_RES_AGAIN) {
		/* Call peekkey() again in force mode to obtain whatever it can */
		/* Don't eat it yet though */
		termkey_peekkey(tk, key, 1, &nbytes);
	}

	return result;
}

TERMKEY_EXPORT TermKeyResult
termkey_getkey_force(TermKey *tk, TermKeyKey *key)
{
	size_t nbytes = 0;
	TermKeyResult result = termkey_peekkey(tk, key, 1, &nbytes);
	if (result == TERMKEY_RES_KEY)
		termkey_eat_bytes(tk, nbytes);
	return result;
}

TERMKEY_EXPORT TermKeyResult
termkey_advisereadable(TermKey *tk)
{
	if (tk->fd == -1) {
		errno = EBADF;
		return TERMKEY_RES_ERROR;
	}

	if (tk->buffstart) {
		memmove(tk->buffer, tk->buffer + tk->buffstart, tk->buffcount);
		tk->buffstart = 0;
	}

	assert(tk->buffcount < sizeof(tk->buffer));

retry:;
	int64_t len = read(tk->fd, tk->buffer + tk->buffcount, sizeof(tk->buffer) - tk->buffcount);

	if (len == -1) {
		if (errno == EAGAIN)
			return TERMKEY_RES_NONE;
		else if (errno == EINTR)
			goto retry;
		else
			return TERMKEY_RES_ERROR;
	} else if (len < 1) {
		tk->is_closed = 1;
		return TERMKEY_RES_NONE;
	}

	tk->buffcount += len;
	return TERMKEY_RES_AGAIN;
}

TERMKEY_EXPORT const char *
termkey_get_keyname(TermKey *tk, TermKeySym sym)
{
	const char *result = "UNKNOWN";
	if (sym != TERMKEY_SYM_UNKNOWN && sym < countof(termkey_keynames))
		result = termkey_keynames[sym];
	return result;
}

static const char *
termkey_lookup_keyname_format(TermKey *tk, const char *str, TermKeySym *sym, TermKeyFormat format)
{
	/* We store an array, so we can't do better than a linear search. Doesn't
	 * matter because user won't be calling this too often */
	const char *result = 0;
	for (*sym = 0; *sym < countof(termkey_keynames); (*sym)++) {
		const char *thiskey = termkey_keynames[*sym];
		size_t len = termkey_keyname_lengths[*sym];
		if (format & TERMKEY_FORMAT_LOWERSPACE) {
			const char *thisstr = str;
			if (termkey_strpncmp_camel(&thisstr, &thiskey, len) == 0) {
				result = thisstr;
				break;
			}
		} else if (strncmp(str, thiskey, len) == 0) {
			result = str + len;
			break;
		}
	}
	return result;
}

static struct termkey_modnames {
	const char *shift, *alt, *ctrl;
} termkey_modnames[] = {
	{"S",     "A",    "C"   }, // 0
	{"Shift", "Alt",  "Ctrl"}, // LONGMOD
	{"S",     "M",    "C"   }, // ALTISMETA
	{"Shift", "Meta", "Ctrl"}, // ALTISMETA+LONGMOD
	{"s",     "a",    "c"   }, // LOWERMOD
	{"shift", "alt",  "ctrl"}, // LOWERMOD+LONGMOD
	{"s",     "m",    "c"   }, // LOWERMOD+ALTISMETA
	{"shift", "meta", "ctrl"}, // LOWERMOD+ALTISMETA+LONGMOD
};

TERMKEY_EXPORT size_t
termkey_strfkey(TermKey *tk, char *buffer, size_t len, TermKeyKey *key, TermKeyFormat format)
{
	size_t pos = 0;
	size_t l   = 0;

	struct termkey_modnames *mods = &termkey_modnames[!!(format & TERMKEY_FORMAT_LONGMOD) +
	                                                  !!(format & TERMKEY_FORMAT_ALTISMETA) * 2 +
	                                                  !!(format & TERMKEY_FORMAT_LOWERMOD) * 4];

	int wrapbracket = (format & TERMKEY_FORMAT_WRAPBRACKET) &&
	                  (key->type != TERMKEY_TYPE_UNICODE || key->modifiers != 0);

	char sep = (format & TERMKEY_FORMAT_SPACEMOD) ? ' ' : '-';

	if (format & TERMKEY_FORMAT_CARETCTRL &&
	    key->type == TERMKEY_TYPE_UNICODE &&
	    key->modifiers == TERMKEY_KEYMOD_CTRL)
	{
		long codepoint = key->code.codepoint;
		// Handle some of the special cases first
		if (codepoint >= 'a' && codepoint <= 'z') {
			l = snprintf(buffer + pos, len - pos, wrapbracket ? "<^%c>" : "^%c", (char)codepoint - 0x20);
			if (l <= 0) return pos;
			pos += l;
			return pos;
		} else if ((codepoint >= '@' && codepoint < 'A') || (codepoint > 'Z' && codepoint <= '_')) {
			l = snprintf(buffer + pos, len - pos, wrapbracket ? "<^%c>" : "^%c", (char)codepoint);
			if (l <= 0) return pos;
			pos += l;
			return pos;
		}
	}

	if (wrapbracket) {
		l = snprintf(buffer + pos, len - pos, "<");
		if (l <= 0) return pos;
		pos += l;
	}

	if (key->modifiers & TERMKEY_KEYMOD_ALT) {
		l = snprintf(buffer + pos, len - pos, "%s%c", mods->alt, sep);
		if (l <= 0) return pos;
		pos += l;
	}

	if (key->modifiers & TERMKEY_KEYMOD_CTRL) {
		l = snprintf(buffer + pos, len - pos, "%s%c", mods->ctrl, sep);
		if (l <= 0) return pos;
		pos += l;
	}

	if (key->modifiers & TERMKEY_KEYMOD_SHIFT) {
		l = snprintf(buffer + pos, len - pos, "%s%c", mods->shift, sep);
		if (l <= 0) return pos;
		pos += l;
	}

	switch(key->type) {
	case TERMKEY_TYPE_UNICODE:{
		if (!key->utf8[0]) // In case of user-supplied key structures
			termkey_fill_utf8(key);
		l = snprintf(buffer + pos, len - pos, "%s", key->utf8);
	}break;

	case TERMKEY_TYPE_KEYSYM:{
		const char *name = termkey_get_keyname(tk, key->code.sym);
		if (format & TERMKEY_FORMAT_LOWERSPACE)
			l = termkey_snprint_cameltospaces(buffer + pos, len - pos, name);
		else
			l = snprintf(buffer + pos, len - pos, "%s", name);
    }break;

	case TERMKEY_TYPE_FUNCTION:{
		l = snprintf(buffer + pos, len - pos, "%c%d",
		             (format & TERMKEY_FORMAT_LOWERSPACE) ? 'f' : 'F', key->code.number);
	}break;

	case TERMKEY_TYPE_MOUSE:{
		TermKeyMouseEvent ev;
		int button;
		int line, col;
		termkey_interpret_mouse(tk, key, &ev, &button, &line, &col);

		l = snprintf(buffer + pos, len - pos, "Mouse%s(%d)", termkey_evnames[ev], button);
		if (format & TERMKEY_FORMAT_MOUSE_POS) {
			if (l <= 0) return pos;
			pos += l;
			l = snprintf(buffer + pos, len - pos, " @ (%u,%u)", col, line);
		}
	}break;

	case TERMKEY_TYPE_POSITION:{l = snprintf(buffer + pos, len - pos, "Position");}break;

	case TERMKEY_TYPE_MODEREPORT:{
		int initial, mode, value;
		termkey_interpret_modereport(tk, key, &initial, &mode, &value);
		if (initial)
			l = snprintf(buffer + pos, len - pos, "Mode(%c%d=%d)", initial, mode, value);
		else
			l = snprintf(buffer + pos, len - pos, "Mode(%d=%d)", mode, value);
	}break;
	case TERMKEY_TYPE_DCS:{
		l = snprintf(buffer + pos, len - pos, "DCS");
	}break;
	case TERMKEY_TYPE_OSC:{l = snprintf(buffer + pos, len - pos, "OSC");}break;

	case TERMKEY_TYPE_UNKNOWN_CSI:{
		l = snprintf(buffer + pos, len - pos, "CSI %c", key->code.number & 0xff);
	}break;
	}

	if (l <= 0) return pos;
	pos += l;

	if (wrapbracket) {
		l = snprintf(buffer + pos, len - pos, ">");
		if (l <= 0) return pos;
		pos += l;
	}

	return pos;
}

TERMKEY_EXPORT const char *
termkey_strpkey(TermKey *tk, const char *str, TermKeyKey *key, TermKeyFormat format)
{
	struct termkey_modnames *mods = &termkey_modnames[!!(format & TERMKEY_FORMAT_LONGMOD) +
	                                                  !!(format & TERMKEY_FORMAT_ALTISMETA) * 2 +
	                                                  !!(format & TERMKEY_FORMAT_LOWERMOD) * 4];

	key->modifiers = 0;

	if ((format & TERMKEY_FORMAT_CARETCTRL) && str[0] == '^' && str[1]) {
		str = termkey_strpkey(tk, str+1, key, format & ~TERMKEY_FORMAT_CARETCTRL);

		if (!str || key->type != TERMKEY_TYPE_UNICODE || key->code.codepoint < '@' ||
		    key->code.codepoint > '_' || key->modifiers != 0)
		{
			return 0;
		}
		if (key->code.codepoint >= 'A' && key->code.codepoint <= 'Z')
			key->code.codepoint += 0x20;
		key->modifiers = TERMKEY_KEYMOD_CTRL;
		termkey_fill_utf8(key);
		return str;
	}

	const char *sep_at;
	while ((sep_at = strchr(str, (format & TERMKEY_FORMAT_SPACEMOD) ? ' ' : '-'))) {
		size_t n = sep_at - str;

		if (n == strlen(mods->alt) && strncmp(mods->alt, str, n) == 0)
			key->modifiers |= TERMKEY_KEYMOD_ALT;
		else if(n == strlen(mods->ctrl) && strncmp(mods->ctrl, str, n) == 0)
			key->modifiers |= TERMKEY_KEYMOD_CTRL;
		else if(n == strlen(mods->shift) && strncmp(mods->shift, str, n) == 0)
			key->modifiers |= TERMKEY_KEYMOD_SHIFT;
		else
			break;
		str = sep_at + 1;
	}

	size_t nbytes;
	int snbytes;
	const char *endstr;
	int button;
	char event_name[32];

	if ((endstr = termkey_lookup_keyname_format(tk, str, &key->code.sym, format))) {
		key->type = TERMKEY_TYPE_KEYSYM;
		str = endstr;
	} else if (sscanf(str, "F%d%n", &key->code.number, &snbytes) == 1) {
		key->type = TERMKEY_TYPE_FUNCTION;
		str += snbytes;
	} else if (sscanf(str, "Mouse%31[^(](%d)%n", event_name, &button, &snbytes) == 2) {
		str += snbytes;
		key->type = TERMKEY_TYPE_MOUSE;

		TermKeyMouseEvent ev = TERMKEY_MOUSE_UNKNOWN;
		for (size_t i = 0; i < countof(termkey_evnames); i++) {
			if(strcmp(termkey_evnames[i], event_name) == 0) {
				ev = TERMKEY_MOUSE_UNKNOWN + i;
				break;
			}
		}

		int code;
		switch(ev) {
		default:{code = 128;}break;
		case TERMKEY_MOUSE_RELEASE:{code = 3;}break;
		case TERMKEY_MOUSE_PRESS:
		case TERMKEY_MOUSE_DRAG:
		{
			code = button - 1;
			if (ev == TERMKEY_MOUSE_DRAG)
				code |= 0x20;
		}break;
		}
		key->code.mouse[0] = code;

		unsigned int line = 0, col = 0;
		if ((format & TERMKEY_FORMAT_MOUSE_POS) && sscanf(str, " @ (%u,%u)%n", &col, &line, &snbytes) == 2) {
			str += snbytes;
		}
		termkey_key_set_linecol(key, col, line);
	// Unicode must be last
	} else if (termkey_parse_utf8((unsigned const char *)str, strlen(str), &key->code.codepoint, &nbytes) == TERMKEY_RES_KEY) {
		key->type = TERMKEY_TYPE_UNICODE;
		termkey_fill_utf8(key);
		str += nbytes;
	} else {
		return NULL;
	}

	termkey_canonicalise(tk, key);
	return str;
}
