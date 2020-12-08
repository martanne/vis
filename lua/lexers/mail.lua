local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'mail'}

-- Headers.
local h = l.alnum + '-'
local header = token(l.KEYWORD, l.starts_line(h^1) * ':')

-- Quotes.  For a discussion of reply quoting, see
-- https://en.wikipedia.org/wiki/Posting_style#Quoted_line_prefix
-- and https://en.wikipedia.org/wiki/Usenet_quoting
-- Although ">" is more common, we also add other chars, also
-- in other editors.
local q = S('>|]}') * (l.space - l.newline)^0
local quote1 = token(l.COMMENT, l.starts_line(q) * l.nonnewline^0)
local quote2 = token(l.PREPROCESSOR, l.starts_line(q^2) * l.nonnewline^0)
local quote3 = token(l.COMMENT, l.starts_line(q^3) * l.nonnewline^0)
local quote4 = token(l.PREPROCESSOR, l.starts_line(q^4) * l.nonnewline^0)
local quote5 = token(l.COMMENT, l.starts_line(q^5) * l.nonnewline^0)
local quote6 = token(l.PREPROCESSOR, l.starts_line(q^6) * l.nonnewline^0)

M._rules = {
  {'quote6', quote6},
  {'quote5', quote5},
  {'quote4', quote4},
  {'quote3', quote3},
  {'quote2', quote2},
  {'quote1', quote1},
  {'header', header},
}

return M
