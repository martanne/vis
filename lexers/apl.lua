-- APL LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'apl'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '⍝' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"')

local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, P('⍞') + (P('⎕') * l.alpha^0))

-- Variables.
local variable = token(l.VARIABLE, (l.alpha + S('_∆⍙')) * (l.alnum + S('_∆⍙¯')^0))

-- Operators.
local operator = token(l.OPERATOR, S('{}[]()←→'))

-- Labels.
local label = token(l.LABEL, l.alnum^1 * P(':'))

-- Nabla.
local nabla = token('nabla', S('∇⍫'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'keyword', keyword},
  {'label', label},
  {'variable', variable},
  {'operator', operator},
  {'nabla', nabla},
}

M._tokenstyles = {

}

return M
