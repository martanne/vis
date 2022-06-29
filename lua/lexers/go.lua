-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Go LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('go')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'break', 'case', 'chan', 'const', 'continue', 'default', 'defer', 'else', 'fallthrough', 'for',
  'func', 'go', 'goto', 'if', 'import', 'interface', 'map', 'package', 'range', 'return', 'select',
  'struct', 'switch', 'type', 'var'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match('true false iota nil')))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bool', 'byte', 'complex64', 'complex128', 'error', 'float32', 'float64', 'int', 'int8', 'int16',
  'int32', 'int64', 'rune', 'string', 'uint', 'uint8', 'uint16', 'uint32', 'uint64', 'uintptr'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'append', 'cap', 'close', 'complex', 'copy', 'delete', 'imag', 'len', 'make', 'new', 'panic',
  'print', 'println', 'real', 'recover'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local raw_str = lexer.range('`', false, false)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + raw_str))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * P('i')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-*/%&|^<>=!:;.,()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
