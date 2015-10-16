-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Forth LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'forth'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = S('|\\') * l.nonnewline^0
local block_comment = '(*' * (l.any - '*)')^0 * P('*)')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local s_str = 's' * l.delimited_range('"', true, true)
local dot_str = '.' * l.delimited_range('"', true, true)
local f_str = 'f' * l.delimited_range('"', true, true)
local dq_str = l.delimited_range('"', true, true)
local string = token(l.STRING, s_str + dot_str + f_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, P('-')^-1 * l.digit^1 * (S('./') * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'swap', 'drop', 'dup', 'nip', 'over', 'rot', '-rot', '2dup', '2drop', '2over',
  '2swap', '>r', 'r>',
  'and', 'or', 'xor', '>>', '<<', 'not', 'negate', 'mod', '/mod', '1+', '1-',
  'base', 'hex', 'decimal', 'binary', 'octal',
  '@', '!', 'c@', 'c!', '+!', 'cell+', 'cells', 'char+', 'chars',
  'create', 'does>', 'variable', 'variable,', 'literal', 'last', '1,', '2,',
  '3,', ',', 'here', 'allot', 'parse', 'find', 'compile',
  -- Operators.
  'if', '=if', '<if', '>if', '<>if', 'then', 'repeat', 'until', 'forth', 'macro'
}, '2><1-@!+3,='))

-- Identifiers.
local identifier = token(l.IDENTIFIER, (l.alnum + S('+-*=<>.?/\'%,_$'))^1)

-- Operators.
local operator = token(l.OPERATOR, S(':;<>+*-/()[]'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'string', string},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
