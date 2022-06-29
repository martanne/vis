-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Props LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('props', {lex_by_line = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Colors.
local xdigit = lexer.xdigit
lex:add_rule('color', token('color', '#' * xdigit * xdigit * xdigit * xdigit * xdigit * xdigit))
lex:add_style('color', lexer.styles.number)

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Equals.
lex:add_rule('equals', token(lexer.OPERATOR, '='))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, '$' * lexer.range('(', ')', true)))

return lex
