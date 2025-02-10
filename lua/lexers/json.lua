-- Copyright 2006-2025 Brian "Sir Alaran" Schott. See LICENSE.
-- JSON LPeg lexer.
-- Based off of lexer code by Mitchell.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lexer.word_match('true false null')))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('[]{}:,')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

return lex
