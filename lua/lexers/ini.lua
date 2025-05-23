-- Copyright 2006-2025 Mitchell. See LICENSE.
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
local integer = S('+-')^-1 * (lexer.hex_num + lexer.oct_num_('_') + lexer.dec_num_('_'))
lex:add_rule('number', token(lexer.NUMBER, lexer.float + integer))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, '='))

lexer.property['scintillua.comment'] = '#'

return lex
