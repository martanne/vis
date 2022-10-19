
-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Markdown LPeg lexer.
-- Copyright 2020 Haelwenn (lanodan) Monnier <contact+gemini.lua@hacktivis.me>
-- Gemini / Gemtext LPeg lexer.
-- See https://gemini.circumlunar.space/docs/specification.html

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local lex = lexer.new('gemini')

local header = token('h3', lexer.starts_line('###') * lexer.nonnewline^0) +
               token('h2', lexer.starts_line('##') * lexer.nonnewline^0) +
               token('h1', lexer.starts_line('#') * lexer.nonnewline^0)
lex:add_rule('header', header)
lex:add_style('h1', {fore = lexer.colors.red, size = 15})
lex:add_style('h2', {fore = lexer.colors.red, size = 14})
lex:add_style('h3', {fore = lexer.colors.red, size = 13})

local list = token('list', lexer.starts_line('*') * lexer.nonnewline^0)
lex:add_rule('list', list)
lex:add_style('list', lexer.styles.constant)

local blockquote = token(lexer.STRING, lexer.starts_line('>') * lexer.nonnewline^0)
lex:add_rule('blockquote', blockquote)

-- Should only match ``` at start of line
local pre = token('pre', lexer.range('```', false, true))
lex:add_rule('pre', pre)
lex:add_style('pre', lexer.styles.embedded .. {eolfilled = true})

-- Whitespace.
local ws = token(lexer.WHITESPACE, S(' \t')^1 + S('\v\r\n')^1)
lex:add_rule('whitespace', ws)

local link = token('link', lexer.starts_line('=>') * lexer.nonnewline^0)
lex:add_rule('link', link)
lex:add_style('link', {underlined=true})

return lex
