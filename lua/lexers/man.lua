-- Copyright 2015-2022 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- man/roff LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('man')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Markup.
lex:add_rule('rule1', token(lexer.STRING, '.' * lexer.to_eol('B' * P('R')^-1 + 'I' * P('PR')^-1)))
lex:add_rule('rule2', token(lexer.NUMBER, lexer.to_eol('.' * S('ST') * 'H')))
lex:add_rule('rule3', token(lexer.KEYWORD, P('.br') + '.DS' + '.RS' + '.RE' + '.PD'))
lex:add_rule('rule4', token(lexer.LABEL, '.' * (S('ST') * 'H' + '.TP')))
lex:add_rule('rule5', token(lexer.VARIABLE, '.B' * P('R')^-1 + '.I' * S('PR')^-1 + '.PP'))
lex:add_rule('rule6', token(lexer.TYPE, '\\f' * S('BIPR')))
lex:add_rule('rule7', token(lexer.PREPROCESSOR, lexer.starts_line('.') * lexer.alpha^1))

return lex
