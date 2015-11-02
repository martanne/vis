/** configure your desired default key bindings */
#define ALIAS(name) .alias = name,
#define ACTION(id) .action = &vis_action[VIS_ACTION_##id],

static KeyBinding basic_movement[] = {
	{ "<C-z>",         ACTION(EDITOR_SUSPEND)                  },
	{ "<Left>",        ACTION(CURSOR_CHAR_PREV)                },
	{ "<S-Left>",      ACTION(CURSOR_LONGWORD_START_PREV)      },
	{ "<Right>",       ACTION(CURSOR_CHAR_NEXT)                },
	{ "<S-Right>",     ACTION(CURSOR_LONGWORD_START_NEXT)      },
	{ "<Up>",          ACTION(CURSOR_LINE_UP)                  },
	{ "<Down>",        ACTION(CURSOR_LINE_DOWN)                },
	{ "<PageUp>",      ACTION(WINDOW_PAGE_UP)                  },
	{ "<PageDown>",    ACTION(WINDOW_PAGE_DOWN)                },
	{ "<S-PageUp>",    ACTION(WINDOW_HALFPAGE_UP)              },
	{ "<S-PageDown>",  ACTION(WINDOW_HALFPAGE_DOWN)            },
	{ "<Home>",        ACTION(CURSOR_LINE_BEGIN)               },
	{ "<End>",         ACTION(CURSOR_LINE_END)                 },
	{ /* empty last element, array terminator */               },
};

static KeyBinding vis_movements[] = {
	{ "h",             ACTION(CURSOR_CHAR_PREV)                },
	{ "<Backspace>",   ALIAS("h")                              },
	{ "<DEL>",         ALIAS("<Backspace>")                    },
	{ "<C-h>",         ALIAS("<Backspace>")                    },
	{ "l",             ACTION(CURSOR_CHAR_NEXT)                },
	{ "<Space>",       ALIAS("l")                              },
	{ "k",             ACTION(CURSOR_LINE_UP)                  },
	{ "C-p",           ALIAS("k")                              },
	{ "j",             ACTION(CURSOR_LINE_DOWN)                },
	{ "<C-j>",         ALIAS("j")                              },
	{ "<C-n>",         ALIAS("j")                              },
	{ "<Enter>",       ALIAS("j")                              },
	{ "gk",            ACTION(CURSOR_SCREEN_LINE_UP)           },
	{ "g<Up>",         ALIAS("gk")                             },
	{ "gj",            ACTION(CURSOR_SCREEN_LINE_DOWN)         },
	{ "g<Down>",       ALIAS("gj")                             },
	{ "^",             ACTION(CURSOR_LINE_START)               },
	{ "g_",            ACTION(CURSOR_LINE_FINISH)              },
	{ "$",             ACTION(CURSOR_LINE_END)                 },
	{ "%",             ACTION(CURSOR_BRACKET_MATCH)            },
	{ "b",             ACTION(CURSOR_WORD_START_PREV)          },
	{ "B",             ACTION(CURSOR_LONGWORD_START_PREV)      },
	{ "w",             ACTION(CURSOR_WORD_START_NEXT)          },
	{ "W",             ACTION(CURSOR_LONGWORD_START_NEXT)      },
	{ "ge",            ACTION(CURSOR_WORD_END_PREV)            },
	{ "gE",            ACTION(CURSOR_LONGWORD_END_PREV)        },
	{ "e",             ACTION(CURSOR_WORD_END_NEXT)            },
	{ "E",             ACTION(CURSOR_LONGWORD_END_NEXT)        },
	{ "{",             ACTION(CURSOR_PARAGRAPH_PREV)           },
	{ "}",             ACTION(CURSOR_PARAGRAPH_NEXT)           },
	{ "(",             ACTION(CURSOR_SENTENCE_PREV)            },
	{ ")",             ACTION(CURSOR_SENTENCE_NEXT)            },
	{ "[[",            ACTION(CURSOR_FUNCTION_START_PREV)      },
	{ "[]",            ACTION(CURSOR_FUNCTION_END_PREV)        },
	{ "][",            ACTION(CURSOR_FUNCTION_START_NEXT)      },
	{ "]]",            ACTION(CURSOR_FUNCTION_END_NEXT)        },
	{ "gg",            ACTION(CURSOR_LINE_FIRST)               },
	{ "g0",            ACTION(CURSOR_SCREEN_LINE_BEGIN)        },
	{ "gm",            ACTION(CURSOR_SCREEN_LINE_MIDDLE)       },
	{ "g$",            ACTION(CURSOR_SCREEN_LINE_END)          },
	{ "G",             ACTION(CURSOR_LINE_LAST)                },
	{ "|",             ACTION(CURSOR_COLUMN)                   },
	{ "n",             ACTION(CURSOR_SEARCH_NEXT)              },
	{ "N",             ACTION(CURSOR_SEARCH_PREV)              },
	{ "H",             ACTION(CURSOR_WINDOW_LINE_TOP)          },
	{ "M",             ACTION(CURSOR_WINDOW_LINE_MIDDLE)       },
	{ "L",             ACTION(CURSOR_WINDOW_LINE_BOTTOM)       },
	{ "*",             ACTION(CURSOR_SEARCH_WORD_FORWARD)      },
	{ "#",             ACTION(CURSOR_SEARCH_WORD_BACKWARD)     },
	{ "f",             ACTION(TO_RIGHT)                        },
	{ "F",             ACTION(TO_LEFT)                         },
	{ "t",             ACTION(TILL_RIGHT)                      },
	{ "T",             ACTION(TILL_LEFT)                       },
	{ ";",             ACTION(TOTILL_REPEAT)                   },
	{ ",",             ACTION(TOTILL_REVERSE)                  },
	{ "/",             ACTION(PROMPT_SEARCH_FORWARD)           },
	{ "?",             ACTION(PROMPT_SEARCH_BACKWARD)          },
	{ "`",             ACTION(MARK_GOTO)                       },
	{ "'",             ACTION(MARK_GOTO_LINE)                  },
	{ /* empty last element, array terminator */               },
};

static KeyBinding vis_textobjs[] = {
	{ "aw",  ACTION(TEXT_OBJECT_WORD_OUTER)                  },
	{ "aW",  ACTION(TEXT_OBJECT_LONGWORD_OUTER)              },
	{ "as",  ACTION(TEXT_OBJECT_SENTENCE)                    },
	{ "ap",  ACTION(TEXT_OBJECT_PARAGRAPH)                   },
	{ "a[",  ACTION(TEXT_OBJECT_SQUARE_BRACKET_OUTER)        },
	{ "a]",  ALIAS("a[")                                     },
	{ "a(",  ACTION(TEXT_OBJECT_PARANTHESE_OUTER)            },
	{ "a)",  ALIAS("a(")                                     },
	{ "ab",  ALIAS("a(")                                     },
	{ "a<",  ACTION(TEXT_OBJECT_ANGLE_BRACKET_OUTER)         },
	{ "a>",  ALIAS("a<")                                     },
	{ "a{",  ACTION(TEXT_OBJECT_CURLY_BRACKET_OUTER)         },
	{ "a}",  ALIAS("a{")                                     },
	{ "aB",  ALIAS("a{")                                     },
	{ "a\"", ACTION(TEXT_OBJECT_QUOTE_OUTER)                 },
	{ "a\'", ACTION(TEXT_OBJECT_SINGLE_QUOTE_OUTER)          },
	{ "a`",  ACTION(TEXT_OBJECT_BACKTICK_OUTER)              },
	{ "ae",  ACTION(TEXT_OBJECT_ENTIRE_OUTER)                },
	{ "af",  ACTION(TEXT_OBJECT_FUNCTION_OUTER)              },
	{ "al",  ACTION(TEXT_OBJECT_LINE_OUTER)                  },
	{ "iw",  ACTION(TEXT_OBJECT_WORD_INNER)                  },
	{ "iW",  ACTION(TEXT_OBJECT_LONGWORD_INNER)              },
	{ "is",  ACTION(TEXT_OBJECT_SENTENCE)                    },
	{ "ip",  ACTION(TEXT_OBJECT_PARAGRAPH)                   },
	{ "i[",  ACTION(TEXT_OBJECT_SQUARE_BRACKET_INNER)        },
	{ "i]",  ALIAS("i[")                                     },
	{ "i(",  ACTION(TEXT_OBJECT_PARANTHESE_INNER)            },
	{ "i)",  ALIAS("i(")                                     },
	{ "ib",  ALIAS("ib")                                     },
	{ "i<",  ACTION(TEXT_OBJECT_ANGLE_BRACKET_INNER)         },
	{ "i>",  ALIAS("i<")                                     },
	{ "i{",  ACTION(TEXT_OBJECT_CURLY_BRACKET_INNER)         },
	{ "i}",  ALIAS("i{")                                     },
	{ "iB",  ALIAS("i{")                                     },
	{ "i\"", ACTION(TEXT_OBJECT_QUOTE_INNER)                 },
	{ "i\'", ACTION(TEXT_OBJECT_SINGLE_QUOTE_INNER)          },
	{ "i`",  ACTION(TEXT_OBJECT_BACKTICK_INNER)              },
	{ "ie",  ACTION(TEXT_OBJECT_ENTIRE_INNER)                },
	{ "if",  ACTION(TEXT_OBJECT_FUNCTION_INNER)              },
	{ "il",  ACTION(TEXT_OBJECT_LINE_INNER)                  },
	{ /* empty last element, array terminator */             },
};

static KeyBinding vis_operators[] = {
	{ "0",              ACTION(COUNT)                             },
	{ "1",              ACTION(COUNT)                             },
	{ "2",              ACTION(COUNT)                             },
	{ "3",              ACTION(COUNT)                             },
	{ "4",              ACTION(COUNT)                             },
	{ "5",              ACTION(COUNT)                             },
	{ "6",              ACTION(COUNT)                             },
	{ "7",              ACTION(COUNT)                             },
	{ "8",              ACTION(COUNT)                             },
	{ "9",              ACTION(COUNT)                             },
	{ "d",              ACTION(OPERATOR_DELETE)                   },
	{ "c",              ACTION(OPERATOR_CHANGE)                   },
	{ "y",              ACTION(OPERATOR_YANK)                     },
	{ "p",              ACTION(PUT_AFTER)                         },
	{ "P",              ACTION(PUT_BEFORE)                        },
	{ "gp",             ACTION(PUT_AFTER_END)                     },
	{ "gP",             ACTION(PUT_BEFORE_END)                    },
	{ ">",              ACTION(OPERATOR_SHIFT_RIGHT)              },
	{ "<",              ACTION(OPERATOR_SHIFT_LEFT)               },
	{ "gU",             ACTION(OPERATOR_CASE_UPPER)               },
	{ "~",              ACTION(OPERATOR_CASE_SWAP)                },
	{ "g~",             ACTION(OPERATOR_CASE_SWAP)                },
	{ "gu",             ACTION(OPERATOR_CASE_LOWER)               },
	{ "\"",             ACTION(REGISTER)                          },
	{ /* empty last element, array terminator */                  },
};

static KeyBinding vis_operator_options[] = {
	{ "v",    ACTION(MOTION_CHARWISE)                               },
	{ "V",    ACTION(MOTION_LINEWISE)                               },
	{ /* empty last element, array terminator */                    },
};

static KeyBinding vis_mode_normal[] = {
	{ "<Escape>",         ACTION(CURSORS_REMOVE_ALL)                    },
	{ "<C-k>",            ACTION(CURSORS_NEW_LINE_ABOVE)                },
	{ "<C-j>",            ACTION(CURSORS_NEW_LINE_BELOW)                },
	{ "<C-a>",            ACTION(CURSORS_ALIGN)                         },
	{ "<C-n>",            ACTION(CURSOR_SELECT_WORD)                    },
	{ "<C-p>",            ACTION(CURSORS_REMOVE_LAST)                   },
	{ "<C-w>n",           ALIAS(":open<Enter>")                         },
	{ "<C-w>c",           ALIAS(":q<Enter>")                            },
	{ "<C-w>s",           ALIAS(":split<Enter>")                        },
	{ "<C-w>v",           ALIAS(":vsplit<Enter>")                       },
	{ "<C-w>j",           ACTION(WINDOW_NEXT)                           },
	{ "<C-w>l",           ALIAS("<C-w>j")                               },
	{ "<C-w><C-w>",       ALIAS("<C-w>j")                               },
	{ "<C-w><C-j>",       ALIAS("<C-w>j")                               },
	{ "<C-w><C-l>",       ALIAS("<C-w>j")                               },
	{ "<C-w>k",           ACTION(WINDOW_PREV)                           },
	{ "<C-w>h",           ALIAS("<C-w>k")                               },
	{ "<C-w><C-h>",       ALIAS("<C-w>k")                               },
	{ "<C-w><C-k>",       ALIAS("<C-w>k")                               },
	{ "<C-w><Backspace>", ALIAS("<C-w>k")                               },
	{ "<C-w><DEL>",       ALIAS("<C-w><Backspace>")                     },
	{ "<C-b>",            ALIAS("<PageUp>")                             },
	{ "<C-f>",            ALIAS("<PageDown>")                           },
	{ "<C-u>",            ALIAS("<S-PageUp>")                           },
	{ "<C-d>",            ALIAS("<S-PageDown>")                         },
	{ "<C-e>",            ACTION(WINDOW_SLIDE_UP)                       },
	{ "<C-y>",            ACTION(WINDOW_SLIDE_DOWN)                     },
	{ "<C-o>",            ACTION(JUMPLIST_PREV)                         },
	{ "<C-i>",            ACTION(JUMPLIST_NEXT)                         },
	{ "g;",               ACTION(CHANGELIST_PREV)                       },
	{ "g,",               ACTION(CHANGELIST_NEXT)                       },
	{ "a",                ACTION(APPEND_CHAR_NEXT)                      },
	{ "A",                ACTION(APPEND_LINE_END)                       },
	{ "C",                ALIAS("c$")                                   },
	{ "D",                ALIAS("d$")                                   },
	{ "I",                ACTION(INSERT_LINE_START)                     },
	{ ".",                ACTION(REPEAT)                                },
	{ "o",                ACTION(OPEN_LINE_BELOW)                       },
	{ "O",                ACTION(OPEN_LINE_ABOVE)                       },
	{ "J",                ACTION(JOIN_LINE_BELOW)                       },
	{ "x",                ACTION(DELETE_CHAR_NEXT)                      },
	{ "r",                ACTION(REPLACE_CHAR)                          },
	{ "i",                ACTION(MODE_INSERT)                           },
	{ "v",                ACTION(MODE_VISUAL)                           },
	{ "V",                ACTION(MODE_VISUAL_LINE)                      },
	{ "R",                ACTION(MODE_REPLACE)                          },
	{ "S",                ALIAS("cc")                                   },
	{ "s",                ALIAS("cl")                                   },
	{ "Y",                ALIAS("yy")                                   },
	{ "X",                ALIAS("dh")                                   },
	{ "u",                ACTION(UNDO)                                  },
	{ "<C-r>",            ACTION(REDO)                                  },
	{ "g+",               ACTION(LATER)                                 },
	{ "g-",               ACTION(EARLIER)                               },
	{ "<C-l>",            ACTION(REDRAW)                                },
	{ ":",                ACTION(PROMPT_SHOW)                           },
	{ "ZZ",               ALIAS(":wq<Enter>")                           },
	{ "ZQ",               ALIAS(":q!<Enter>")                           },
	{ "zt",               ACTION(WINDOW_REDRAW_TOP)                     },
	{ "zz",               ACTION(WINDOW_REDRAW_CENTER)                  },
	{ "zb",               ACTION(WINDOW_REDRAW_BOTTOM)                  },
	{ "q",                ACTION(MACRO_RECORD)                          },
	{ "@",                ACTION(MACRO_REPLAY)                          },
	{ "gv",               ACTION(SELECTION_RESTORE)                     },
	{ "m",                ACTION(MARK_SET)                              },
	{ /* empty last element, array terminator */                        },
};

static KeyBinding vis_mode_visual[] = {
	{ "<C-n>",              ACTION(CURSORS_NEW_MATCH_NEXT)                },
	{ "<C-x>",              ACTION(CURSORS_NEW_MATCH_SKIP)                },
	{ "<C-p>",              ACTION(CURSORS_REMOVE_LAST)                   },
	{ "I",                  ACTION(CURSORS_NEW_LINES_BEGIN)               },
	{ "A",                  ACTION(CURSORS_NEW_LINES_END)                 },
	{ "<Backspace>",        ALIAS("d")                                    },
	{ "<C-h>",              ALIAS("<Backspace>")                          },
	{ "<DEL>",              ALIAS("<Backspace>")                          },
	{ "<Escape>",           ACTION(MODE_NORMAL)                           },
	{ "<C-c>",              ALIAS("<Escape>")                             },
	{ "v",                  ALIAS("<Escape>")                             },
	{ "V",                  ACTION(MODE_VISUAL_LINE)                      },
	{ ":",                  ACTION(PROMPT_SHOW_VISUAL)                    },
	{ "x",                  ALIAS("d")                                    },
	{ "r",                  ALIAS("c")                                    },
	{ "s",                  ALIAS("c")                                    },
	{ "J",                  ACTION(JOIN_LINES)                            },
	{ "o",                  ACTION(SELECTION_FLIP)                        },
	{ /* empty last element, array terminator */                          },
};

static KeyBinding vis_mode_visual_line[] = {
	{ "v",      ACTION(MODE_VISUAL)                                   },
	{ "V",      ACTION(MODE_NORMAL)                                   },
	{ /* empty last element, array terminator */                      },
};

static KeyBinding vis_mode_readline[] = {
	{ "<Backspace>",    ACTION(DELETE_CHAR_PREV)                    },
	{ "<DEL>",          ALIAS("<Backspace>")                        },
	{ "<C-h>",          ALIAS("<Backspace>")                        },
	{ "<Escape>",       ACTION(MODE_NORMAL)                         },
	{ "<C-c>",          ALIAS("<Enter>")                            },
	{ "<C-d>",          ACTION(DELETE_CHAR_NEXT)                    },
	{ "<C-w>",          ACTION(DELETE_WORD_PREV)                    },
	{ "<C-u>",          ACTION(DELETE_LINE_BEGIN)                   },
	{ /* empty last element, array terminator */                    },
};

static KeyBinding vis_mode_prompt[] = {
	{ "<Backspace>",    ACTION(PROMPT_BACKSPACE)                    },
	{ "<DEL>",          ALIAS("<Backspace>")                        },
	{ "<C-h>",          ALIAS("<Backspace>")                        },
	{ "<Enter>",        ACTION(PROMPT_ENTER)                        },
	{ "<C-j>",          ALIAS("<Enter>")                            },
	{ "<Tab>",          ACTION(NOP)                                 },
	{ /* empty last element, array terminator */                    },
};

static KeyBinding vis_mode_insert[] = {
	{ "<Escape>",           ACTION(MODE_NORMAL)                         },
	{ "<C-c>",              ALIAS("<Escape>")                           },
	{ "<C-i>",              ALIAS("<Tab>")                              },
	{ "<Enter>",            ACTION(INSERT_NEWLINE)                      },
	{ "<C-j>",              ALIAS("<Enter>")                            },
	{ "<C-m>",              ALIAS("<Enter>")                            },
	{ "<C-o>",              ACTION(MODE_OPERATOR_PENDING)               },
	{ "<C-v>",              ACTION(INSERT_VERBATIM)                     },
	{ "<C-d>",              ALIAS("<Escape><<i")                        },
	{ "<C-t>",              ALIAS("<Escape>>>i")                        },
	{ "<C-x><C-e>",         ACTION(WINDOW_SLIDE_UP)                     },
	{ "<C-x><C-y>",         ACTION(WINDOW_SLIDE_DOWN)                   },
	{ "<Tab>",              ACTION(INSERT_TAB)                          },
	{ "<C-r>",              ACTION(INSERT_REGISTER)                     },
	{ /* empty last element, array terminator */                        },
};

static KeyBinding vis_mode_replace[] = {
	{ /* empty last element, array terminator */                           },
};

/* Color definitions for use in the sytax highlighting rules below. A fore
 * or background color of -1 specifies the default terminal color. */
enum {
	COLOR_NOHILIT,
	COLOR_SYNTAX0,
	COLOR_SYNTAX1,
	COLOR_SYNTAX2,
	COLOR_SYNTAX3,
	COLOR_SYNTAX4,
	COLOR_SYNTAX5,
	COLOR_SYNTAX6,
	COLOR_SYNTAX7,
	COLOR_SYNTAX8,
	COLOR_SYNTAX9,
	COLOR_SYNTAX_LAST, /* below are only aliases */
	COLOR_KEYWORD      = COLOR_SYNTAX1,
	COLOR_CONSTANT     = COLOR_SYNTAX4,
	COLOR_DATATYPE     = COLOR_SYNTAX2,
	COLOR_OPERATOR     = COLOR_SYNTAX2,
	COLOR_CONTROL      = COLOR_SYNTAX3,
	COLOR_PREPROCESSOR = COLOR_SYNTAX4,
	COLOR_PRAGMA       = COLOR_SYNTAX4,
	COLOR_KEYWORD2     = COLOR_SYNTAX4,
	COLOR_BRACKETS     = COLOR_SYNTAX5,
	COLOR_STRING       = COLOR_SYNTAX6,
	COLOR_LITERAL      = COLOR_SYNTAX6,
	COLOR_VARIABLE     = COLOR_SYNTAX6,
	COLOR_TARGET       = COLOR_SYNTAX5,
	COLOR_COMMENT      = COLOR_SYNTAX7,
	COLOR_IDENTIFIER   = COLOR_SYNTAX8,
	COLOR_TYPE         = COLOR_SYNTAX9,
	COLOR_WHITESPACE   = COLOR_COMMENT,
	COLOR_SPACES       = COLOR_WHITESPACE,
	COLOR_TABS         = COLOR_WHITESPACE,
	COLOR_EOL          = COLOR_WHITESPACE,
	COLOR_EOF          = COLOR_WHITESPACE,
};

static const char *styles[] = {
	[COLOR_NOHILIT] = "",
	[COLOR_SYNTAX0] = "fore:red,bold",
	[COLOR_SYNTAX1] = "fore:green,bold",
	[COLOR_SYNTAX2] = "fore:green",
	[COLOR_SYNTAX3] = "fore:magenta,bold",
	[COLOR_SYNTAX4] = "fore:magenta",
	[COLOR_SYNTAX5] = "fore:blue,bold",
	[COLOR_SYNTAX6] = "fore:red",
	[COLOR_SYNTAX7] = "fore:blue",
	[COLOR_SYNTAX8] = "fore:cyan",
	[COLOR_SYNTAX9] = "fore:yellow",
	[COLOR_SYNTAX_LAST] = NULL,
};

/* Syntax color definitions per file type. Each rule consists of a regular
 * expression, a color to apply in case of a match and boolean flag inidcating
 * whether it is a multiline rule.
 *
 * The syntax rules where initially imported from the sandy editor, written by
 * Rafael Garcia  <rafael.garcia.gallego@gmail.com>
 */
#define B "\\b"
/* Use this if \b is not in your libc's regex implementation */
// #define B "^| |\t|\\(|\\)|\\[|\\]|\\{|\\}|\\||$"

/* common rules, used by multiple languages */

#define SYNTAX_MULTILINE_COMMENT {    \
	"(/\\*([^*]|\\*+[^*/])*\\*+/|/\\*([^*]|\\*+[^*/])*$|^([^/]|/+[^/*])*\\*/)", \
	COLOR_COMMENT,                \
	true, /* multiline */         \
}

#define SYNTAX_SINGLE_LINE_COMMENT { \
	"(//.*)",                    \
	COLOR_COMMENT,               \
}

#define SYNTAX_LITERAL {                             \
	"('(\\\\.|.)')|"B"(0x[0-9A-Fa-f]+|[0-9]+)"B, \
	COLOR_LITERAL,                               \
}

#define SYNTAX_STRING {         \
	"(\"(\\\\.|[^\"])*\")", \
	COLOR_STRING,           \
	false, /* multiline */  \
}

#define SYNTAX_CONSTANT {        \
	B"[A-Z_][0-9A-Z_]+"B,    \
	COLOR_CONSTANT,          \
}

#define SYNTAX_BRACKET {             \
	"(\\(|\\)|\\{|\\}|\\[|\\])", \
	COLOR_BRACKETS,              \
}

#define SYNTAX_C_PREPROCESSOR { \
	"(^#[\\t ]*(define|include(_next)?|(un|ifn?)def|endif|el(if|se)|if|warning|error|pragma)?)", \
	COLOR_PREPROCESSOR, \
}

#define SYNTAX_SPACES    { "\xC2\xB7",     COLOR_SPACES }
#define SYNTAX_TABS      { "\xE2\x96\xB6", COLOR_TABS   }
#define SYNTAX_TABS_FILL { " ",            COLOR_TABS   }
#define SYNTAX_EOL       { "\xE2\x8F\x8E", COLOR_EOL    }
#define SYNTAX_EOF       { "~",            COLOR_EOF    }

/* these rules are applied top to bottom, first match wins. Therefore more 'greedy'
 * rules such as for comments should be the first entries.
 *
 * The array of syntax definition must be terminated with an empty element.
 */
static Syntax syntaxes[] = {{
	.name = "c",
	.file = "\\.(c(pp|xx)?|h(pp|xx)?|cc)$",
	.settings = (const char*[]){
		"set number",
		"set autoindent",
		"set show spaces=0 tabs=1 newlines=1",
		NULL
	},
	.styles = styles,
	.symbols = {
		SYNTAX_SPACES,
		SYNTAX_TABS,
		SYNTAX_TABS_FILL,
		SYNTAX_EOL,
		SYNTAX_EOF,
	},
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		"<[a-zA-Z0-9\\.\\-_/]+\\.(c(pp|xx)?|h(pp|xx)?|cc)>",
		COLOR_STRING,
	},
		SYNTAX_C_PREPROCESSOR,
	{
		B"(for|if|while|do|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		COLOR_KEYWORD,
	},{
		B"(float|double|bool|char|int|short|long|sizeof|enum|void|static|const|struct|union|"
		"typedef|extern|(un)?signed|inline|((s?size)|((u_?)?int(8|16|32|64|ptr)))_t|class|"
		"namespace|template|public|protected|private|typename|this|friend|virtual|using|"
		"mutable|volatile|register|explicit)"B,
		COLOR_DATATYPE,
	},{
		B"(goto|continue|break|return)"B,
		COLOR_CONTROL,
	}}
},{
	.name = "sh",
	.file = "\\.sh$",
	.styles = styles,
	.rules = {{
		"#.*$",
		COLOR_COMMENT,
	},
		SYNTAX_STRING,
	{
		"^[0-9A-Z_]+\\(\\)",
		COLOR_CONSTANT,
	},{
		"\\$[?!@#$?*-]",
		COLOR_VARIABLE,
	},{
		"\\$\\{[A-Za-z_][0-9A-Za-z_]+\\}",
		COLOR_VARIABLE,
	},{
		"\\$[A-Za-z_][0-9A-Za-z_]+",
		COLOR_VARIABLE,
	},{
		B"(case|do|done|elif|else|esac|exit|fi|for|function|if|in|local|read|return|select|shift|then|time|until|while)"B,
		COLOR_KEYWORD,
	},{
		"(\\{|\\}|\\(|\\)|\\;|\\]|\\[|`|\\\\|\\$|<|>|!|=|&|\\|)",
		COLOR_BRACKETS,
	}}
},{
	.name = "makefile",
	.file = "(Makefile[^/]*|\\.mk)$",
	.styles = styles,
	.rules = {{
		"#.*$",
		COLOR_COMMENT,
	},{
		"\\$+[{(][a-zA-Z0-9_-]+[})]",
		COLOR_VARIABLE,
	},{
		B"(if|ifeq|else|endif)"B,
		COLOR_CONTROL,
	},{
		"^[^ 	]+:",
		COLOR_TARGET,
	},{
		"[:(+?=)]",
		COLOR_BRACKETS,
	}}
},{
	.name = "man",
	.file = "\\.[1-9]x?$",
	.styles = styles,
	.rules = {{
		"\\.(BR?|I[PR]?).*$",
		COLOR_SYNTAX0,
	},{
		"\\.(S|T)H.*$",
		COLOR_SYNTAX2,
	},{
		"\\.(br|DS|RS|RE|PD)",
		COLOR_SYNTAX3,
	},{
		"(\\.(S|T)H|\\.TP)",
		COLOR_SYNTAX4,
	},{
		"\\.(BR?|I[PR]?|PP)",
		COLOR_SYNTAX5,
	},{
		"\\\\f[BIPR]",
		COLOR_SYNTAX6,
	}}
},{
	.name = "vala",
	.file = "\\.(vapi|vala)$",
	.styles = styles,
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		B"(for|if|while|do|else|case|default|switch|get|set|value|out|ref|enum)"B,
		COLOR_KEYWORD,
	},{
		B"(uint|uint8|uint16|uint32|uint64|bool|byte|ssize_t|size_t|char|double|string|float|int|long|short|this|base|transient|void|true|false|null|unowned|owned)"B,
		COLOR_DATATYPE,
	},{
		B"(try|catch|throw|finally|continue|break|return|new|sizeof|signal|delegate)"B,
		COLOR_CONTROL,
	},{
		B"(abstract|class|final|implements|import|instanceof|interface|using|private|public|static|strictfp|super|throws)"B,
		COLOR_KEYWORD2,
	}}
},{
	.name = "java",
	.file = "\\.java$",
	.styles = styles,
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_CONSTANT,
		SYNTAX_BRACKET,
	{
		B"(for|if|while|do|else|case|default|switch)"B,
		COLOR_KEYWORD,
	},{
		B"(boolean|byte|char|double|float|int|long|short|transient|void|true|false|null)"B,
		COLOR_DATATYPE,
	},{
		B"(try|catch|throw|finally|continue|break|return|new)"B,
		COLOR_CONTROL,
	},{
		B"(abstract|class|extends|final|implements|import|instanceof|interface|native|package|private|protected|public|static|strictfp|this|super|synchronized|throws|volatile)"B,
		COLOR_KEYWORD2,
	}}
},{
	.name = "javascript",
	.file = "\\.(js|json)$",
	.styles = styles,
	.rules = {
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_LITERAL,
		SYNTAX_STRING,
		SYNTAX_BRACKET,
	{
		B"(true|false|null|undefined)"B,
		COLOR_DATATYPE,
	},{
		B"(NaN|Infinity)"B,
		COLOR_LITERAL,
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		COLOR_STRING,
	},{
		B"(for|if|while|do|in|else|case|default|switch|try|throw|catch|operator|new|delete)"B,
		COLOR_KEYWORD,
	},{
		B"(continue|break|return)"B,
		COLOR_CONTROL,
	},{
		B"(case|class|const|debugger|default|enum|export|extends|finally|function|implements|import|instanceof|let|this|typeof|var|with|yield)"B,
		COLOR_KEYWORD2,
	}}
},{
	.name = "lua",
	.file = "\\.lua$",
	.settings = (const char*[]){
		"set number",
		"set autoindent",
		NULL
	},
	.styles = styles,
	.rules = {{
		"--\\[(=*)\\[([^]]*)\\](=*)\\]",
		COLOR_COMMENT,
		true,
	},{
		"--.*$",
		COLOR_COMMENT,
	},{
		"(\\[(=*)\\[([^]]*)\\](=*)\\]|^([^][]*)\\](=*)\\])",
		COLOR_STRING,
		true,
	},
		SYNTAX_STRING,
	{
		B"([0-9]*\\.)?[0-9]+([eE]([\\+-])?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"0x[0-9a-fA-F]+"B,
		COLOR_LITERAL,
	},{
		B"(false|nil|true)"B,
		COLOR_CONSTANT,
	},{
		"(\\.\\.\\.)",
		COLOR_CONSTANT,
	},{
		B"(break|do|else|elseif|end|for|function|if|in|local|repeat|return|then|until|while)"B,
		COLOR_KEYWORD,
	},{
		B"(and|not|or)"B,
		COLOR_OPERATOR,
	},{
		"(\\+|-|\\*|/|%|\\^|#|[=~<>]=|<|>|\\.\\.)",
		COLOR_OPERATOR,
	},
		SYNTAX_BRACKET,
	}
},{
	.name = "ruby",
	.file = "\\.rb$",
	.styles = styles,
	.rules = {{
		"(#[^{].*$|#$)",
		COLOR_COMMENT,
	},{
		"(\\$|@|@@)?"B"[A-Z]+[0-9A-Z_a-z]*",
		COLOR_VARIABLE,
	},{
		B"(__FILE__|__LINE__|BEGIN|END|alias|and|begin|break|case|class|def|defined\?|do|else|elsif|end|ensure|false|for|if|in|module|next|nil|not|or|redo|rescue|retry|return|self|super|then|true|undef|unless|until|when|while|yield)"B,
		COLOR_KEYWORD,
	},{
		"([ 	]|^):[0-9A-Z_]+"B,
		COLOR_SYNTAX2,
	},{
		"(/([^/]|(\\/))*/[iomx]*|%r\\{([^}]|(\\}))*\\}[iomx]*)",
		COLOR_SYNTAX3,
	},{
		"(`[^`]*`|%x\\{[^}]*\\})",
		COLOR_SYNTAX4,
	},{
		"(\"([^\"]|(\\\\\"))*\"|%[QW]?\\{[^}]*\\}|%[QW]?\\([^)]*\\)|%[QW]?<[^>]*>|%[QW]?\\[[^]]*\\]|%[QW]?\\$[^$]*\\$|%[QW]?\\^[^^]*\\^|%[QW]?![^!]*!|\'([^\']|(\\\\\'))*\'|%[qw]\\{[^}]*\\}|%[qw]\\([^)]*\\)|%[qw]<[^>]*>|%[qw]\\[[^]]*\\]|%[qw]\\$[^$]*\\$|%[qw]\\^[^^]*\\^|%[qw]![^!]*!)",
		COLOR_SYNTAX5,
	},{
		"#\\{[^}]*\\}",
		COLOR_SYNTAX6,
	}}
},{
	.name = "python",
	.file = "\\.py$",
	.styles = styles,
	.rules = {{
		"(#.*$|#$)",
		COLOR_COMMENT,
	},{
		"(\"\"\".*\"\"\")",
		COLOR_COMMENT,
		true, /* multiline */
	},{
		B"(and|class|def|not|or|return|yield|is)"B,
		COLOR_KEYWORD2,
	},{
		B"(from|import|as)"B,
		COLOR_KEYWORD,
	},{
		B"(if|elif|else|while|for|in|try|with|except|in|break|continue|finally)"B,
		COLOR_CONTROL,
	},{
		B"(int|str|float|unicode|int|bool|chr|type|list|dict|tuple)",
		COLOR_DATATYPE,
	},{
		"(True|False|None)",
		COLOR_LITERAL,
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		COLOR_LITERAL,
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		COLOR_STRING,
		false, /* multiline */  
	},{
		"(__init__|__str__|__unicode__|__gt__|__lt__|__eq__|__enter__|__exit__|__next__|__getattr__|__getitem__|__setitem__|__call__|__contains__|__iter__|__bool__|__all__|__name__)",
		COLOR_SYNTAX2,
	}}
},{
	.name = "php",
	.file = "\\.php$",
	.styles = styles,
	.rules = {
		SYNTAX_MULTILINE_COMMENT,
		SYNTAX_SINGLE_LINE_COMMENT,
		SYNTAX_BRACKET,
	{
		"(#.*$|#$)",
		COLOR_COMMENT,
	},{
		"(\"\"\".*\"\"\")",
		COLOR_COMMENT,
		true, /* multiline */
	},{
		B"(class|interface|extends|implements|new|__construct|__destruct|use|namespace|return)"B,
		COLOR_KEYWORD2,
	},{
		B"(public|private|protected|const|parent|function|->)"B,
		COLOR_KEYWORD,
	},{
		B"(if|else|while|do|for|foreach|in|try|catch|finally|switch|case|default|break|continue|as|=>)"B,
		COLOR_CONTROL,
	},{
		B"(array|true|false|null)",
		COLOR_DATATYPE,
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		COLOR_LITERAL,
	},{
		"\\$[a-zA-Z0-9_\\-]+",
		COLOR_VARIABLE,
	},{
		"(\"(\\\\.|[^\"])*\"|\'(\\\\.|[^\'])*\')",
		COLOR_STRING,
		false, /* multiline */
	},{
		"(php|echo|print|var_dump|print_r)",
		COLOR_SYNTAX2,
	}}
},{
	.name = "haskell",
	.file = "\\.hs$",
	.styles = styles,
	.rules = {{
		"\\{-#.*#-\\}",
		COLOR_PRAGMA,
	},{
		"---*([^-!#$%&\\*\\+./<=>\?@\\^|~].*)?$",
		COLOR_COMMENT,
	}, {
		// These are allowed to be nested, but we can't express that
		// with regular expressions
		"\\{-.*-\\}",
		COLOR_COMMENT,
		true
	},
		SYNTAX_STRING,
		SYNTAX_C_PREPROCESSOR,
	{
		// as and hiding are only keywords when part of an import, but
		// I don't want to highlight the whole import line.
		// capture group coloring or similar would be nice
		"(^import( qualified)?)|"B"(as|hiding|infix[lr]?)"B,
		COLOR_KEYWORD2,
	},{
		B"(module|class|data|deriving|instance|default|where|type|newtype)"B,
		COLOR_KEYWORD,
	},{
		B"(do|case|of|let|in|if|then|else)"B,
		COLOR_CONTROL,
	},{
		"('(\\\\.|.)')",
		COLOR_LITERAL,
	},{
		B"[0-9]+\\.[0-9]+([eE][-+]?[0-9]+)?"B,
		COLOR_LITERAL,
	},{
		B"[0-9]+"B"|"B"0[xX][0-9a-fA-F]+"B"|"B"0[oO][0-7]+"B,
		COLOR_LITERAL,
	},{
		"("B"[A-Z][a-zA-Z0-9_']*\\.)*"B"[a-zA-Z][a-zA-Z0-9_']*"B,
		COLOR_NOHILIT,
	},{
		"("B"[A-Z][a-zA-Z0-9_']*\\.)?[-!#$%&\\*\\+/<=>\\?@\\\\^|~:.][-!#$%&\\*\\+/<=>\\?@\\\\^|~:.]*",
		COLOR_OPERATOR,
	},{
		"`("B"[A-Z][a-zA-Z0-9_']*\\.)?[a-z][a-zA-Z0-9_']*`",
		COLOR_OPERATOR,
	},{
		"\\(|\\)|\\[|\\]|,|;|_|\\{|\\}",
		COLOR_BRACKETS,
	}}
},{
	.name = "markdown",
	.file = "\\.(md|mdwn)$",
	.styles = styles,
	.rules = {{
		"(^#{1,6}.*$)", //titles
		COLOR_SYNTAX5,
	},{
		"((\\* *){3,}|(_ *){3,}|(- *){3,})", // horizontal rules
		COLOR_SYNTAX2,
	},{
		"(\\*\\*.*\\*\\*)|(__.*__)", // super-bolds
		COLOR_SYNTAX4,
	},{
		"(\\*.*\\*)|(_.*_)", // bolds
		COLOR_SYNTAX3,
	},{
		"(\\[.*\\]\\(.*\\))", //links
		COLOR_SYNTAX6,
	},{
		"(^ *([-\\*\\+]|[0-9]+\\.))", //lists
		COLOR_SYNTAX2,
	},{
		"(^( {4,}|\t+).*$)", // code blocks
		COLOR_SYNTAX7,
	},{
		"(`+.*`+)", // inline code
		COLOR_SYNTAX7,
	},{
		"(^>+.*)", // quotes
		COLOR_SYNTAX7,
	}}
},{
	.name = "ledger",
	.file = "\\.(journal|ledger)$",
	.styles = styles,
	.rules = {
	{ /* comment */
		"^[;#].*",
		COLOR_COMMENT,
	},{ /* value tag */
		"(  |\t|^ )*; :([^ ][^:]*:)+[ \\t]*$",
		COLOR_DATATYPE,
	},{ /* typed tag */
		"(  |\t|^ )*; [^:]+::.*",
		COLOR_DATATYPE,
	},{ /* tag */
		"(  |\t|^ )*; [^:]+:.*",
		COLOR_TYPE,
	},{ /* metadata */
		"(  |\t|^ )*;.*",
		COLOR_CONSTANT,
	},{ /* date */
		"^[0-9][^ \t]+",
		COLOR_LITERAL,
	},{ /* account */
		"^[ \t]+[a-zA-Z:'!*()%&]+",
		COLOR_IDENTIFIER,
	},{ /* amount */
		"(  |\t)[^;]*",
		COLOR_LITERAL,
	},{ /* automated transaction */
		"^[=~].*",
		COLOR_TYPE,
	},{ /* directives */
		"^[!@]?(account|alias|assert|bucket|capture|check|comment|commodity|define|end|fixed|endfixed|include|payee|apply|tag|test|year|[AYNDCIiOobh])"B".*",
		COLOR_DATATYPE,
	}}
},{
	.name = "apl",
	.file = "\\.apl$",
	.settings = (const char*[]){
		"set number",
		NULL
	},
	.styles = styles,
	.rules = {{
		"(⍝|#).*$",
		COLOR_COMMENT,
	},{
		"('([^']|'')*')|(\"([^\"]|\"\")*\")",
		COLOR_STRING,
	},{
		"^ *(∇|⍫)",
		COLOR_SYNTAX9,
	},{
		"(⎕[a-zA-Z]*)|[⍞χ⍺⍶⍵⍹]",
		COLOR_KEYWORD,
	},{
		"[∆⍙_a-zA-Z][∆⍙_¯a-zA-Z0-9]* *:",
		COLOR_SYNTAX2,
	},{
		"[∆⍙_a-zA-Z][∆⍙_¯a-zA-Z0-9]*",
		COLOR_IDENTIFIER,
	},{
		"¯?(([0-9]+(\\.[0-9]+)?)|\\.[0-9]+)([eE]¯?[0-9]+)?([jJ]¯?(([0-9]+(\\.[0-9]+)?)|\\.[0-9]+)([eE]¯?[0-9]+)?)?",
		COLOR_CONSTANT,
	},{
		"[][(){}]",
		COLOR_BRACKETS,
	},{
		"[←→◊]",
		COLOR_SYNTAX3,
	}}
},{
	/* empty last element, array terminator */
}};
