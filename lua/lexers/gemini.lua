-- Copyright 2020-2025 Haelwenn (lanodan) Monnier <contact+gemini.lua@hacktivis.me>. See LICENSE.
-- Gemini / Gemtext LPeg lexer.
-- See https://gemini.circumlunar.space/docs/specification.html

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

local header = lex:tag(lexer.HEADING .. '.h3', lexer.to_eol(lexer.starts_line('###'))) +
	lex:tag(lexer.HEADING .. '.h2', lexer.to_eol(lexer.starts_line('##'))) +
	lex:tag(lexer.HEADING .. '.h1', lexer.to_eol(lexer.starts_line('#')))
lex:add_rule('header', header)

lex:add_rule('list', lex:tag(lexer.LIST, lexer.to_eol(lexer.starts_line('*'))))

lex:add_rule('blockquote', lex:tag(lexer.STRING, lexer.to_eol(lexer.starts_line('>'))))

lex:add_rule('pre', lex:tag(lexer.CODE, lexer.to_eol(lexer.range('```', false, true))))

lex:add_rule('link', lex:tag(lexer.LINK, lexer.to_eol(lexer.starts_line('=>'))))

return lex
