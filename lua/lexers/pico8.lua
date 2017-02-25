-- Copyright 2016-2017 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- PICO-8 Lexer.
-- http://www.lexaloffle.com/pico-8.php

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'pico8'}

-- Whitespace
local ws = token(l.WHITESPACE, l.space^1)

-- Comments
local comment = token(l.COMMENT, '//' * l.nonnewline_esc^0)

-- Numbers
local number = token(l.NUMBER, l.integer)

-- Keywords
local keyword = token(l.KEYWORD, word_match{
  '__lua__', '__gfx__', '__gff__', '__map__', '__sfx__', '__music__'
})

-- Identifiers
local identifier = token(l.IDENTIFIER, l.word)

-- Operators
local operator = token(l.OPERATOR, S('_'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

-- Embed Lua into PICO-8.
local lua = l.load('lua')

local lua_start_rule = token('pico8_tag', '__lua__')
local lua_end_rule = token('pico8_tag', '__gfx__' )
l.embed_lexer(M, lua, lua_start_rule, lua_end_rule)

M._tokenstyles = {
  pico8_tag = l.STYLE_EMBEDDED
}

M._foldsymbols = lua._foldsymbols

return M
