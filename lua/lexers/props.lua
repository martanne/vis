-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Props LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'props'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Equals.
local equals = token(l.OPERATOR, '=')

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local string = token(l.STRING, sq_str + dq_str)

-- Variables.
local variable = token(l.VARIABLE, '$(' * (l.any - ')')^1 * ')')

-- Colors.
local xdigit = l.xdigit
local color = token('color', '#' * xdigit * xdigit * xdigit * xdigit * xdigit *
                             xdigit)

M._rules = {
  {'whitespace', ws},
  {'color', color},
  {'comment', comment},
  {'equals', equals},
  {'string', string},
  {'variable', variable},
}

M._tokenstyles = {
  color = l.STYLE_NUMBER
}

M._LEXBYLINE = true

return M
