-- Copyright 2016-2025 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- PICO-8 lexer.
-- http://www.lexaloffle.com/pico-8.php

local lexer = lexer
local word_match = lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords
lex:add_rule('keyword',
	lex:tag(lexer.KEYWORD, lexer.word_match('__gff__ __map__ __sfx__ __music__')))

-- Identifiers
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('//', true)))

-- Numbers
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.integer))

-- Operators
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '_'))

-- Embed Lua into PICO-8.
local lua = lexer.load('lua')
local lua_start_rule = lex:tag(lexer.KEYWORD, word_match('__lua__'))
local lua_end_rule = lex:tag(lexer.KEYWORD, word_match('__gfx__'))
lex:embed(lua, lua_start_rule, lua_end_rule)

lexer.property['scintillua.comment'] = '//'

return lex
