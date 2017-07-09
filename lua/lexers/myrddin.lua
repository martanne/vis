-- Copyright 2017 Michael Forney. See LICENSE
-- Myrddin LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = {_NAME = 'myrddin'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = P{
  V'part' * P'*/'^-1,
  part = '/*' * (V'full' + (l.any - '/*' - '*/'))^0,
  full = V'part' * '*/',
}
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local digit = l.digit + '_'
local bdigit = R'01' + '_'
local xdigit = l.xdigit + '_'
local odigit = R'07' + '_'
local integer = '0x' * xdigit^1 + '0o' * odigit^1 + '0b' * bdigit^1 + digit^1
local float = digit^1 * (('.' * digit^1) * (S'eE' * S'+-'^-1 * digit^1)^-1 +
                         ('.' * digit^1)^-1 * S'eE' * S'+-'^-1 * digit^1)
local number = token(l.NUMBER, float + integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'break', 'const', 'continue', 'elif', 'else', 'extern', 'false', 'for',
  'generic', 'goto', 'if', 'impl', 'in', 'match', 'pkg', 'pkglocal', 'sizeof',
  'struct', 'trait', 'true', 'type', 'union', 'use', 'var', 'while',
})

-- Types.
local type = token(l.TYPE, word_match{
  'void', 'bool', 'char', 'byte',
  'int8', 'uint8',
  'int16', 'uint16',
  'int32', 'uint32',
  'int64', 'uint64',
  'int', 'uint',
  'flt32', 'flt64',
} + '@' * l.word)

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S'`#_+-/*%<>~!=^&|~:;,.()[]{}')

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
