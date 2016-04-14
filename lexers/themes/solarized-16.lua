-- Solarized color codes Copyright (c) 2011 Ethan Schoonover
-- http://ethanschoonover.com/solarized
-- https://github.com/altercation/solarized
--
-- SOLARIZED HEX     16/8 TERMCOL  XTERM/HEX   L*A*B      sRGB        HSB
-- --------- ------- ---- -------  ----------- ---------- ----------- -----------
-- base03    #002b36  8/4 brblack  234 #1c1c1c 15 -12 -12   0  43  54 193 100  21
-- base02    #073642  0/4 black    235 #262626 20 -12 -12   7  54  66 192  90  26
-- base01    #586e75 10/7 brgreen  240 #4e4e4e 45 -07 -07  88 110 117 194  25  46
-- base00    #657b83 11/7 bryellow 241 #585858 50 -07 -07 101 123 131 195  23  51
-- base0     #839496 12/6 brblue   244 #808080 60 -06 -03 131 148 150 186  13  59
-- base1     #93a1a1 14/4 brcyan   245 #8a8a8a 65 -05 -02 147 161 161 180   9  63
-- base2     #eee8d5  7/7 white    254 #d7d7af 92 -00  10 238 232 213  44  11  93
-- base3     #fdf6e3 15/7 brwhite  230 #ffffd7 97  00  10 253 246 227  44  10  99
-- yellow    #b58900  3/3 yellow   136 #af8700 60  10  65 181 137   0  45 100  71
-- orange    #cb4b16  9/3 brred    166 #d75f00 50  50  55 203  75  22  18  89  80
-- red       #dc322f  1/1 red      160 #d70000 50  65  45 220  50  47   1  79  86
-- magenta   #d33682  5/5 magenta  125 #af005f 50  65 -05 211  54 130 331  74  83
-- violet    #6c71c4 13/5 brmagenta 61 #5f5faf 50  15 -45 108 113 196 237  45  77
-- blue      #268bd2  4/4 blue      33 #0087ff 55 -10 -45  38 139 210 205  82  82
-- cyan      #2aa198  6/6 cyan      37 #00afaf 60 -35 -05  42 161 152 175  74  63
-- green     #859900  2/2 green     64 #5f8700 60 -20  65 133 153   0  68 100  60

local lexers = vis.lexers

local colors = {
	['base03']  = '8',
	['base02']  = '0',
	['base01']  = '10',
	['base00']  = '11',
	['base0']   = '12',
	['base1']   = '14',
	['base2']   = '7',
	['base3']   = '15',
	['yellow']  = '3',
	['orange']  = '9',
	['red']     = '1',
	['magenta'] = '5',
	['violet']  = '13',
	['blue']    = '4',
	['cyan']    = '6',
	['green']   = '2',
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
lexers.STYLE_CLASS = 'fore:'..colors.yellow
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
lexers.STYLE_EMBEDDED = 'fore:'..colors.blue
lexers.STYLE_IDENTIFIER = fg

lexers.STYLE_LINENUMBER = fg
lexers.STYLE_CURSOR = 'fore:'..colors.base03..',back:'..colors.base0
lexers.STYLE_CURSOR_PRIMARY = lexers.STYLE_CURSOR..',blink'
lexers.STYLE_CURSOR_LINE = 'back:'..colors.base02
lexers.STYLE_COLOR_COLUMN = 'back:'..colors.base02
lexers.STYLE_SELECTION = 'back:'..colors.base3
