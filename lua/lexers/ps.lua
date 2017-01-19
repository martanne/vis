-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Postscript LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'ps'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '%' * l.nonnewline^0)

-- Strings.
local arrow_string = l.delimited_range('<>')
local nested_string = l.delimited_range('()', false, false, true)
local string = token(l.STRING, arrow_string + nested_string)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'pop', 'exch', 'dup', 'copy', 'roll', 'clear', 'count', 'mark', 'cleartomark',
  'counttomark', 'exec', 'if', 'ifelse', 'for', 'repeat', 'loop', 'exit',
  'stop', 'stopped', 'countexecstack', 'execstack', 'quit', 'start',
  'true', 'false', 'NULL'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'add', 'div', 'idiv', 'mod', 'mul', 'sub', 'abs', 'ned', 'ceiling', 'floor',
  'round', 'truncate', 'sqrt', 'atan', 'cos', 'sin', 'exp', 'ln', 'log', 'rand',
  'srand', 'rrand'
})

-- Identifiers.
local word = (l.alpha + '-') * (l.alnum + '-')^0
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('[]{}'))

-- Labels.
local label = token(l.LABEL, '/' * word)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'label', label},
  {'operator', operator},
}

return M
