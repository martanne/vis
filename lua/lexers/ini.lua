-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Ini LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ini')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match('true false on off yes no')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '_') * (lexer.alnum + S('_.'))^0))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Labels.
lex:add_rule('label', token(lexer.LABEL, lexer.range('[', ']', true)))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol(lexer.starts_line(S(';#')))))

-- Numbers.
local dec = lexer.digit^1 * ('_' * lexer.digit^1)^0
local oct_num = '0' * S('01234567_')^1
local integer = S('+-')^-1 * (lexer.hex_num + oct_num + dec)
lex:add_rule('number', token(lexer.NUMBER, lexer.float + integer))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, '='))

return lex
