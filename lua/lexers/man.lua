-- Copyright 2015-2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- man/roff LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'man'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Markup.
local rule1 = token(l.STRING,
                    P('.') * (P('B') * P('R')^-1 + P('I') * P('PR')^-1) *
                    l.nonnewline^0)
local rule2 = token(l.NUMBER, P('.') * S('ST') * P('H') * l.nonnewline^0)
local rule3 = token(l.KEYWORD,
                    P('.br') + P('.DS') + P('.RS') + P('.RE') + P('.PD'))
local rule4 = token(l.LABEL, P('.') * (S('ST') * P('H') + P('.TP')))
local rule5 = token(l.VARIABLE,
                    P('.B') * P('R')^-1 + P('.I') * S('PR')^-1 + P('.PP'))
local rule6 = token(l.TYPE, P('\\f') * S('BIPR'))
local rule7 = token(l.PREPROCESSOR, l.starts_line('.') * l.alpha^1)

M._rules = {
  {'whitespace', ws},
  {'rule1', rule1},
  {'rule2', rule2},
  {'rule3', rule3},
  {'rule4', rule4},
  {'rule5', rule5},
  {'rule6', rule6},
  {'rule7', rule7},
}

return M
