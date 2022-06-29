-- Copyright 2006-2022 Mitchell. See LICENSE.
-- IDL LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('idl')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abstract', 'attribute', 'case', 'const', 'context', 'custom', 'default', 'enum', 'exception',
  'factory', 'FALSE', 'in', 'inout', 'interface', 'local', 'module', 'native', 'oneway', 'out',
  'private', 'public', 'raises', 'readonly', 'struct', 'support', 'switch', 'TRUE', 'truncatable',
  'typedef', 'union', 'valuetype'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'any', 'boolean', 'char', 'double', 'fixed', 'float', 'long', 'Object', 'octet', 'sequence',
  'short', 'string', 'unsigned', 'ValueBase', 'void', 'wchar', 'wstring'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Preprocessor.
lex:add_rule('preproc', token(lexer.PREPROCESSOR, lexer.starts_line('#') *
  word_match('define undef ifdef ifndef if elif else endif include warning pragma')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!<>=+-/*%&|^~.,:;?()[]{}')))

return lex
