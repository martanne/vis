-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Props LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {lex_by_line = true})

-- Identifiers.
lex:add_rule('identifier',
	lex:tag(lexer.IDENTIFIER, (lexer.alpha + S('.-_')) * (lexer.alnum + S('.-_')^0)))

-- Colors.
local xdigit = lexer.xdigit
lex:add_rule('color',
	lex:tag(lexer.NUMBER, '#' * xdigit * xdigit * xdigit * xdigit * xdigit * xdigit))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Equals.
lex:add_rule('equals', lex:tag(lexer.OPERATOR, '='))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Variables.
lex:add_rule('variable',
	lex:tag(lexer.OPERATOR, '$(') * lex:tag(lexer.VARIABLE, (lexer.nonnewline - lexer.space - ')')^0) *
		lex:tag(lexer.OPERATOR, ')'))

lexer.property['scintillua.comment'] = '#'

return lex
