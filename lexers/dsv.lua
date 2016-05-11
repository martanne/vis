-- Copyright 2016 Christian Hesse
-- delimiter separated values LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'dsv'}

-- Operators.
local operator = token(l.OPERATOR, S(',;:|'))

M._rules = {
  {'operator', operator}
}

return M
