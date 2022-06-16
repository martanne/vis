-- Copyright 2016-2022 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- PICO-8 lexer.
-- http://www.lexaloffle.com/pico-8.php

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('pico8')

-- Whitespace
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords
lex:add_rule('keyword',
  token(lexer.KEYWORD, word_match('__lua__ __gfx__ __gff__ __map__ __sfx__ __music__')))

-- Identifiers
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('//', true)))

-- Numbers
lex:add_rule('number', token(lexer.NUMBER, lexer.integer))

-- Operators
lex:add_rule('operator', token(lexer.OPERATOR, '_'))

-- Embed Lua into PICO-8.
local lua = lexer.load('lua')
local lua_start_rule = token('pico8_tag', '__lua__')
local lua_end_rule = token('pico8_tag', '__gfx__')
lex:embed(lua, lua_start_rule, lua_end_rule)
lex:add_style('pico8_tag', lexer.styles.embedded)

return lex
