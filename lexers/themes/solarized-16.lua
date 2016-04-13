-- Solarized color codes Copyright (c) 2011 Ethan Schoonover
local lexers = vis.lexers

local colors = {
	['base03']  = '8', -- '#002b36',
	['base02']  = '0', -- '#073642',
	['base01']  = '10', -- '#586e75',
	['base00']  = '11', -- '#657b83',
	['base0']   = '12', -- '#839496',
	['base1']   = '14', -- '#93a1a1',
	['base2']   = '7', -- '#eee8d5',
	['base3']   = '15', -- '#fdf6e3',
	['yellow']  = '3', -- '#b58900',
	['orange']  = '9', -- '#cb4b16',
	['red']     = '1', -- '#dc322f',
	['magenta'] = '5', -- '#d33682',
	['violet']  = '13',  -- '#6c71c4',
	['blue']    = '4',  -- '#268bd2',
	['cyan']    = '6',  -- '#2aa198',
	['green']   = '2',  -- '#859900',
} 

lexers.colors = colors
-- dark
local fg = ',fore:'..colors.base0..','
local bg = ',back:'..colors.base03..','
-- light
-- local fg = ',fore:'..colors.base03..','
-- local bg = ',back:'..colors.base3..','

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
lexers.STYLE_WHITESPACE = ''
lexers.STYLE_EMBEDDED = 'back:blue'
lexers.STYLE_IDENTIFIER = fg

lexers.STYLE_LINENUMBER = fg
lexers.STYLE_CURSOR = 'fore:'..colors.base03..',back:'..colors.base0
lexers.STYLE_CURSOR_PRIMARY = lexers.STYLE_CURSOR..',blink'
lexers.STYLE_CURSOR_LINE = 'back:'..colors.base01
lexers.STYLE_COLOR_COLUMN = 'back:'..colors.base01
-- lexers.STYLE_SELECTION = 'back:'..colors.base02
lexers.STYLE_SELECTION = 'back:white'
