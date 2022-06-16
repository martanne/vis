-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Vala LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('vala')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'class', 'delegate', 'enum', 'errordomain', 'interface', 'namespace', 'signal', 'struct', 'using',
  -- Modifiers.
  'abstract', 'const', 'dynamic', 'extern', 'inline', 'out', 'override', 'private', 'protected',
  'public', 'ref', 'static', 'virtual', 'volatile', 'weak',
  -- Other.
  'as', 'base', 'break', 'case', 'catch', 'construct', 'continue', 'default', 'delete', 'do',
  'else', 'ensures', 'finally', 'for', 'foreach', 'get', 'if', 'in', 'is', 'lock', 'new',
  'requires', 'return', 'set', 'sizeof', 'switch', 'this', 'throw', 'throws', 'try', 'typeof',
  'value', 'var', 'void', 'while',
  -- Etc.
  'null', 'true', 'false'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bool', 'char', 'double', 'float', 'int', 'int8', 'int16', 'int32', 'int64', 'long', 'short',
  'size_t', 'ssize_t', 'string', 'uchar', 'uint', 'uint8', 'uint16', 'uint32', 'uint64', 'ulong',
  'unichar', 'ushort'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local tq_str = lexer.range('"""')
local ml_str = '@' * lexer.range('"', false, false)
lex:add_rule('string', token(lexer.STRING, tq_str + sq_str + dq_str + ml_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('uUlLfFdDmM')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
