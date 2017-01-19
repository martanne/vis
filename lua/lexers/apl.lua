-- Copyright 2015-2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- APL LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'apl'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, (P('⍝') + P('#')) * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"')

local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local dig = R('09')
local rad = P('.')
local exp = S('eE')
local img = S('jJ')
local sgn = P('¯')^-1
local float = sgn * (dig^0 * rad * dig^1 + dig^1 * rad * dig^0 + dig^1) *
              (exp * sgn *dig^1)^-1
local number = token(l.NUMBER, float * img * float + float)

-- Keywords.
local keyword = token(l.KEYWORD, P('⍞') + P('χ') + P('⍺') + P('⍶') + P('⍵') +
                                 P('⍹') + P('⎕') * R('AZ', 'az')^0)

-- Names.
local n1l = R('AZ', 'az')
local n1b = P('_') + P('∆') + P('⍙')
local n2l = n1l + R('09')
local n2b = n1b + P('¯')
local n1 = n1l + n1b
local n2 = n2l + n2b
local name = n1 * n2^0

-- Labels.
local label = token(l.LABEL, name * P(':'))

-- Variables.
local variable = token(l.VARIABLE, name)

-- Special.
local special = token(l.TYPE, S('{}[]();') + P('←') + P('→') + P('◊'))

-- Nabla.
local nabla = token(l.PREPROCESSOR, P('∇') + P('⍫'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'keyword', keyword},
  {'label', label},
  {'variable', variable},
  {'special', special},
  {'nabla', nabla},
}

return M
