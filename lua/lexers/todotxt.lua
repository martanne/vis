-- Copyright 2025 Chris Clark and Mitchell. See LICENSE.
-- todo.txt LPeg lexer.
-- https://github.com/too-much-todotxt/spec

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Done/Complete items.
lex:add_rule('done', lex:tag(lexer.COMMENT, lexer.starts_line(lexer.to_eol('x '))))

-- Priority.
local priority = P(false)
for letter in string.gmatch('abcdefghijklmnopqrstuvwxyz', '.') do
	local tag = lex:tag(lexer.LIST .. '.priority.' .. letter,
		lexer.starts_line('(' .. letter:upper() .. ') '))
	priority = priority + tag
end
lex:add_rule('priority', priority)

-- URLs, emails, and domain names.
-- Note: this is not part of todo.txt, but is an extension to make editing cleaner.
local nonspace = lexer.any - lexer.space
local email = lex:tag(lexer.LINK,
	(nonspace - '@')^1 * '@' * (nonspace - '.')^1 * ('.' * (nonspace - S('.?'))^1)^1 *
		('?' * nonspace^1)^-1)
local host = lex:tag(lexer.LINK,
	lexer.word_match('www ftp', true) * (nonspace - '.')^0 * '.' * (nonspace - '.')^1 * '.' *
		(nonspace - S(',.'))^1)
local url = lex:tag(lexer.LINK,
	(nonspace - '://')^1 * '://' * (nonspace - ',' - '.')^1 * ('.' * (nonspace - S(',./?#'))^1)^1 *
		('/' * (nonspace - S('./?#'))^0 * ('.' * (nonspace - S(',.?#'))^1)^0)^0 *
		('?' * (nonspace - '#')^1)^-1 * ('#' * nonspace^0)^-1)
local link = url + host + email
lex:add_rule('link', link)

-- Key-value pairs.
local word = (lexer.any - lexer.space - P(':'))^1
local key = lex:tag(lexer.KEYWORD, word)
local colon = lex:tag(lexer.OPERATOR, P(':'))
local value = lex:tag(lexer.STRING, word)
lex:add_rule('key_value', key * colon * value)

-- Dates.
lex:add_rule('date', lex:tag(lexer.NUMBER, lexer.digit^4 * P('-') * lexer.digit^2 * P('-') *
	lexer.digit^2 * (#lexer.space + P(-1))))

-- Project +
lex:add_rule('project', lex:tag(lexer.LABEL, lexer.range('+', lexer.space, true)))
-- Context @
lex:add_rule('context', lex:tag(lexer.TYPE, lexer.range('@', lexer.space, true)))

return lex
