-- Copyright 2017-2022 Michael Forney. See LICENSE
-- Myrddin LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('myrddin')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'break', 'const', 'continue', 'elif', 'else', 'extern', 'false', 'for', 'generic', 'goto', 'if',
  'impl', 'in', 'match', 'pkg', 'pkglocal', 'sizeof', 'struct', 'trait', 'true', 'type', 'union',
  'use', 'var', 'while'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'void', 'bool', 'char', 'byte', 'int', 'uint', 'int8', 'uint8', 'int16', 'uint16', 'int32',
  'uint32', 'int64', 'uint64', 'flt32', 'flt64'
} + '@' * lexer.word))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Numbers.
local digit = lexer.digit + '_'
local bdigit = S('01') + '_'
local xdigit = lexer.xdigit + '_'
local odigit = lpeg.R('07') + '_'
local integer = '0x' * xdigit^1 + '0o' * odigit^1 + '0b' * bdigit^1 + digit^1
local float = digit^1 * ((('.' * digit^1) * (S('eE') * S('+-')^-1 * digit^1)^-1) +
  (('.' * digit^1)^-1 * S('eE') * S('+-')^-1 * digit^1))
lex:add_rule('number', token(lexer.NUMBER, float + integer))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('`#_+-/*%<>~!=^&|~:;,.()[]{}')))

return lex
