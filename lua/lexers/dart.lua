-- Copyright 2013-2022 Mitchell. See LICENSE.
-- Dart LPeg lexer.
-- Written by Brian Schott (@Hackerpilot on Github).

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('dart')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'assert', 'break', 'case', 'catch', 'class', 'const', 'continue', 'default', 'do', 'else', 'enum',
  'extends', 'false', 'final', 'finally', 'for', 'if', 'in', 'is', 'new', 'null', 'rethrow',
  'return', 'super', 'switch', 'this', 'throw', 'true', 'try', 'var', 'void', 'while', 'with'
}))

-- Built-ins.
lex:add_rule('builtin', token(lexer.CONSTANT, word_match{
  'abstract', 'as', 'dynamic', 'export', 'external', 'factory', 'get', 'implements', 'import',
  'library', 'operator', 'part', 'set', 'static', 'typedef'
}))

-- Strings.
local sq_str = S('r')^-1 * lexer.range("'", true)
local dq_str = S('r')^-1 * lexer.range('"', true)
local tq_str = S('r')^-1 * (lexer.range("'''") + lexer.range('"""'))
lex:add_rule('string', token(lexer.STRING, tq_str + sq_str + dq_str))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('#?=!<>+-*$/%&|^~.,;()[]{}')))

-- Annotations.
lex:add_rule('annotation', token('annotation', '@' * lexer.word^1))
lex:add_style('annotation', lexer.styles.preprocessor)

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
