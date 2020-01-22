-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Text LPeg lexer.

local l = require('lexer')

local M = {_NAME = 'text'}

-- Whitespace.
local ws = l.token(l.WHITESPACE, l.space^1)

M._rules = {
  {'whitespace', ws},
}

return M
