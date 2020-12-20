-- Copyright 2006-2020 Mitchell. See LICENSE.
-- R LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rstats')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  break else for if in next repeat return switch try while
  Inf NA NaN NULL FALSE TRUE
]]))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match[[
  array character complex data.frame double factor function integer list logical
  matrix numeric vector
]]))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * P('i')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('<->+*/^=.,:;|$()[]{}')))

return lex
