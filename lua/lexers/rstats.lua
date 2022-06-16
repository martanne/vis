-- Copyright 2006-2022 Mitchell. See LICENSE.
-- R LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rstats')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'break', 'else', 'for', 'if', 'in', 'next', 'repeat', 'return', 'switch', 'try', 'while', --
  'Inf', 'NA', 'NaN', 'NULL', 'FALSE', 'TRUE', 'F', 'T',
  -- Frequently used operators.
  '|>', '%%', '%*%', '%/%', '%in%', '%o%', '%x%'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'array', 'character', 'closure', 'complex', 'data.frame', 'double', 'environment', 'expression',
  'externalptr', 'factor', 'function', 'integer', 'list', 'logical', 'matrix', 'numeric',
  'pairlist', 'promise', 'raw', 'symbol', 'vector'
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
lex:add_rule('number', token(lexer.NUMBER, (lexer.number * P('i')^-1) * P('L')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('<->+*/^=.,:;|$()[]{}')))

-- Folding
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
