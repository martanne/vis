-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Gtkrc LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'gtkrc'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.digit^1 * ('.' * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'binding', 'class', 'include', 'module_path', 'pixmap_path', 'im_module_file',
  'style', 'widget', 'widget_class'
})

-- Variables.
local variable = token(l.VARIABLE, word_match{
  'bg', 'fg', 'base', 'text', 'xthickness', 'ythickness', 'bg_pixmap', 'font',
  'fontset', 'font_name', 'stock', 'color', 'engine'
})

-- States.
local state = token(l.CONSTANT, word_match{
  'ACTIVE', 'SELECTED', 'NORMAL', 'PRELIGHT', 'INSENSITIVE', 'TRUE', 'FALSE'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'mix', 'shade', 'lighter', 'darker'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.alpha * (l.alnum + S('_-'))^0)

-- Operators.
local operator = token(l.OPERATOR, S(':=,*()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'variable', variable},
  {'state', state},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '#'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
