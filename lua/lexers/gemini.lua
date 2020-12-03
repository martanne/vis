-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Markdown LPeg lexer.
-- Copyright 2020 Haelwenn (lanodan) Monnier <contact+gemini.lua@hacktivis.me>
-- Gemini / Gemtext LPeg lexer.
-- See https://gemini.circumlunar.space/docs/specification.html

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'gemini'}

local ws = token(l.WHITESPACE, S(' \t')^1 + S('\v\r\n')^1)

local link = token('link', l.starts_line('=>') * l.nonnewline^0)

-- Should only match ``` at start of line
local pre = token('pre', l.delimited_range('```', false, true))

local header = token('h3', l.starts_line('###') * l.nonnewline^0) +
               token('h2', l.starts_line('##') * l.nonnewline^0) +
               token('h1', l.starts_line('#') * l.nonnewline^0)

local list = token('list', l.starts_line('*') * l.nonnewline^0)

local blockquote = token(l.STRING, l.starts_line('>') * l.nonnewline^0)

M._rules = {
  {'header', header},
  {'list', list},
  {'blockquote', blockquote},
  {'pre', pre},
  {'whitespace', ws},
  {'link', link}
}

local font_size = 10
local hstyle = 'fore:red'
M._tokenstyles = {
  h3 = hstyle..',size:'..(font_size + 3),
  h2 = hstyle..',size:'..(font_size + 4),
  h1 = hstyle..',size:'..(font_size + 5),
  pre = l.STYLE_EMBEDDED..',eolfilled',
  link = 'underlined',
  list = l.STYLE_CONSTANT
}

return M
