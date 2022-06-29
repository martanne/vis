-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Gap LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('gap')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'and', 'break', 'continue', 'do', 'elif', 'else', 'end', 'fail', 'false', 'fi', 'for', 'function',
  'if', 'in', 'infinity', 'local', 'not', 'od', 'or', 'rec', 'repeat', 'return', 'then', 'true',
  'until', 'while'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.dec_num * -lexer.alpha))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('*+-,./:;<=>~^#()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'function', 'end')
lex:add_fold_point(lexer.KEYWORD, 'do', 'od')
lex:add_fold_point(lexer.KEYWORD, 'if', 'fi')
lex:add_fold_point(lexer.KEYWORD, 'repeat', 'until')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
