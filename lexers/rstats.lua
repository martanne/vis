-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- R LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'rstats'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * P('i')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'break', 'else', 'for', 'if', 'in', 'next', 'repeat', 'return', 'switch',
  'try', 'while', 'Inf', 'NA', 'NaN', 'NULL', 'FALSE', 'TRUE'
})

-- Types.
local type = token(l.TYPE, word_match{
  'array', 'character', 'complex', 'data.frame', 'double', 'factor', 'function',
  'integer', 'list', 'logical', 'matrix', 'numeric', 'vector'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('<->+*/^=.,:;|$()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
