-- Solarized color codes Copyright (c) 2011 Ethan Schoonover
local lexers = vis.lexers

local colors = {
	base03  = '#002b36',
	base02  = '#073642',
	base01  = '#586e75',
	base00  = '#657b83',
	base0   = '#839496',
	base1   = '#93a1a1',
	base2   = '#eee8d5',
	base3   = '#fdf6e3',
	yellow  = '#b58900',
	orange  = '#cb4b16',
	red     = '#dc322f',
	magenta = '#d33682',
	violet  = '#6c71c4',
	blue    = '#268bd2',
	cyan    = '#2aa198',
	green   = '#859900',
}

lexers.colors = colors
-- dark
local fg = ',fore:'..colors.base0..','
local bg = ',back:'..colors.base03..','
-- light
-- local fg = ',fore:'..colors.base03..','
-- local bg = ',back:'..colors.base3..','
-- solarized term
-- local fg = ',fore:default,'
-- local bg = ',back:default,'


lexers.STYLE_DEFAULT = bg..fg
lexers.STYLE_NOTHING = bg
lexers.STYLE_CLASS = 'fore:yellow'
lexers.STYLE_COMMENT = 'fore:'..colors.base01
lexers.STYLE_CONSTANT = 'fore:'..colors.cyan
lexers.STYLE_DEFINITION = 'fore:'..colors.blue
lexers.STYLE_ERROR = 'fore:'..colors.red..',italics'
lexers.STYLE_FUNCTION = 'fore:'..colors.blue
lexers.STYLE_KEYWORD = 'fore:'..colors.green
lexers.STYLE_LABEL = 'fore:'..colors.green
lexers.STYLE_NUMBER = 'fore:'..colors.cyan
lexers.STYLE_OPERATOR = 'fore:'..colors.green
lexers.STYLE_REGEX = 'fore:green'
lexers.STYLE_STRING = 'fore:'..colors.cyan
lexers.STYLE_PREPROCESSOR = 'fore:'..colors.orange
lexers.STYLE_TAG = 'fore:'..colors.red
lexers.STYLE_TYPE = 'fore:'..colors.yellow
lexers.STYLE_VARIABLE = 'fore:'..colors.blue
lexers.STYLE_WHITESPACE = 'fore:'..colors.base01
lexers.STYLE_EMBEDDED = 'back:blue'
lexers.STYLE_IDENTIFIER = fg

lexers.STYLE_LINENUMBER = 'fore:'..colors.base00..',back:'..colors.base02
lexers.STYLE_LINENUMBER_CURSOR = 'back:'..colors.base00..',fore:'..colors.base02
lexers.STYLE_CURSOR = 'fore:'..colors.base03..',back:'..colors.base0
lexers.STYLE_CURSOR_PRIMARY = lexers.STYLE_CURSOR..',back:yellow'
lexers.STYLE_CURSOR_LINE = 'back:'..colors.base02
lexers.STYLE_COLOR_COLUMN = 'back:'..colors.base02
-- lexers.STYLE_SELECTION = 'back:'..colors.base02
lexers.STYLE_SELECTION = 'back:white'
lexers.STYLE_STATUS = 'back:'..colors.base00..',fore:'..colors.base02
lexers.STYLE_STATUS_FOCUSED = 'back:'..colors.base1..',fore:'..colors.base02
lexers.STYLE_SEPARATOR = lexers.STYLE_DEFAULT
lexers.STYLE_INFO = 'fore:default,back:default,bold'
lexers.STYLE_EOF = 'fore:'..colors.base01

-- lexer specific styles

-- Diff
lexers.STYLE_ADDITION = 'fore:green'
lexers.STYLE_DELETION = 'fore:red'
lexers.STYLE_CHANGE = 'fore:yellow'

-- CSS
lexers.STYLE_PROPERTY = lexers.STYLE_ATTRIBUTE
lexers.STYLE_PSEUDOCLASS = ''
lexers.STYLE_PSEUDOELEMENT = ''

-- HTML
lexers.STYLE_TAG_UNKNOWN = lexers.STYLE_TAG .. ',italics'
lexers.STYLE_TAG_SINGLE = lexers.STYLE_TAG
lexers.STYLE_TAG_DOCTYPE = lexers.STYLE_TAG .. ',bold'
lexers.STYLE_ATTRIBUTE_UNKNOWN = lexers.STYLE_ATTRIBUTE .. ',italics'

-- Latex, TeX, and Texinfo
lexers.STYLE_COMMAND = lexers.STYLE_KEYWORD
lexers.STYLE_COMMAND_SECTION = lexers.STYLE_CLASS
lexers.STYLE_ENVIRONMENT = lexers.STYLE_TYPE
lexers.STYLE_ENVIRONMENT_MATH = lexers.STYLE_NUMBER

-- Makefile
lexers.STYLE_TARGET = ''

-- Markdown
lexers.STYLE_HR = ''
for i = 1,6 do lexers['STYLE_HEADING_H'..i] = lexers.STYLE_HEADING end
lexers.STYLE_BOLD = 'bold'
lexers.STYLE_ITALIC = 'italics'
lexers.STYLE_LIST = lexers.STYLE_KEYWORD
lexers.STYLE_LINK = lexers.STYLE_KEYWORD
lexers.STYLE_REFERENCE = lexers.STYLE_KEYWORD
lexers.STYLE_CODE = lexers.STYLE_EMBEDDED

-- Output
lexers.STYE_FILENAME = ''
lexers.STYLE_LINE = ''
lexers.STYLE_COLUMN = ''
lexers.STYLE_MESSAGE = ''

-- Python
lexers.STYLE_KEYWORD_SOFT = ''

-- Taskpaper
lexers.STYLE_NOTE = ''
lexers.STYLE_TAG_EXTENDED = ''
lexers.STYLE_TAG_DAY = 'fore:yellow'
lexers.STYLE_TAG_OVERDUE = 'fore:red'
lexers.STYLE_TAG_PLAIN = ''

-- XML
lexers.STYLE_CDATA = ''

-- YAML
lexers.STYLE_ERROR_INDENT = 'back:red'

-- The following are temporary styles until their legacy lexers are migrated.

-- Antlr
lexers.STYLE_ACTION = ''

-- Clojure
lexers.STYLE_CLOJURE_KEYWORD = lexers.STYLE_TYPE
lexers.STYLE_CLOJURE_SYMBOL = lexers.STYLE_TYPE .. ',bold'

-- Crystal
--lexers.STYLE_SYMBOL = lexers.STYLE_STRING

-- Gleam
lexers.STYLE_MODULE = lexers.STYLE_CONSTANT
lexers.STYLE_DISCARD = lexers.STYLE_COMMENT

-- Icon
lexers.STYLE_SPECIAL_KEYWORD = lexers.STYLE_TYPE

-- jq
lexers.STYLE_FORMAT = lexers.STYLE_CONSTANT
lexers.STYLE_SYSVAR = lexers.STYLE_CONSTANT .. ',bold'

-- Julia
-- lexers.STYLE_SYMBOL = lexers.STYLE_STRING
lexers.STYLE_CHARACTER = lexers.STYLE_CONSTANT

-- Mediawiki
lexers.STYLE_BEHAVIOR_SWITCH = lexers.STYLE_KEYWORD

-- Moonscript
lexers.STYLE_TBL_KEY = lexers.STYLE_REGEX
lexers.STYLE_SELF_REF = lexers.STYLE_LABEL
lexers.STYLE_PROPER_IDENT = lexers.STYLE_CLASS
lexers.STYLE_FNDEF = lexers.STYLE_PREPROCESSOR
-- lexers.STYLE_SYMBOL = lexers.STYLE_EMBEDDED

-- reST
lexers.STYLE_LITERAL_BLOCK = lexers.STYLE_EMBEDDED
lexers.STYLE_FOOTNOTE_BLOCK = lexers.STYLE_LABEL
lexers.STYLE_CITATION_BLOCK = lexers.STYLE_LABEL
lexers.STYLE_LINK_BLOCK = lexers.STYLE_LABEL
lexers.STYLE_CODE_BLOCK = lexers.STYLE_CODE
lexers.STYLE_DIRECTIVE = lexers.STYLE_KEYWORD
lexers.STYLE_SPHINX_DIRECTIVE = lexers.STYLE_KEYWORD
lexers.STYLE_UNKNOWN_DIRECTIVE = lexers.STYLE_KEYWORD
lexers.STYLE_SUBSTITUTION = lexers.STYLE_VARIABLE
lexers.STYLE_INLINE_LITERAL = lexers.STYLE_EMBEDDED
lexers.STYLE_ROLE = lexers.STYLE_CLASS
lexers.STYLE_INTERPRETED = lexers.STYLE_STRING

-- txt2tags
lexers.STYLE_LINE = 'bold'
for i = 1,5 do lexers['STYLE_H'..i] = lexers.STYLE_HEADING end
lexers.STYLE_IMAGE = 'fore:green'
lexers.STYLE_STRIKE = 'italics'
lexers.STYLE_TAGGED = lexers.STYLE_EMBEDDED
lexers.STYLE_TAGGED_AREA = lexers.STYLE_EMBEDDED
lexers.STYLE_TABLE_SEP = 'fore:green'
lexers.STYLE_HEADER_CELL_CONTENT = 'fore:green'
