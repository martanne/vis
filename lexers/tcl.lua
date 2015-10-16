-- Copyright 2014-2015 Joshua Krämer. See LICENSE.
-- Tcl LPeg lexer.
-- This lexer follows the TCL dodekalogue (http://wiki.tcl.tk/10259).
-- It is based on the previous lexer by Mitchell.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'tcl'}

-- Whitespace.
local whitespace = token(l.WHITESPACE, l.space^1)

-- Separator (semicolon).
local separator = token(l.CLASS, P(';'))

-- Delimiters.
local braces = token(l.KEYWORD, S('{}'))
local quotes = token(l.FUNCTION, '"')
local brackets = token(l.VARIABLE, S('[]'))

-- Argument expander.
local expander = token(l.LABEL, P('{*}'))

-- Variable substitution.
local variable = token(l.STRING, '$' * (l.alnum + '_' + P(':')^2)^0)

-- Backslash substitution.
local backslash = token(l.TYPE, '\\' * ((l.digit * l.digit^-2) +
                        ('x' * l.xdigit^1) + ('u' * l.xdigit * l.xdigit^-3) +
                        ('U' * l.xdigit * l.xdigit^-7) + P(1)))

-- Comment.
local comment = token(l.COMMENT, '#' * P(function(input, index)
  local i = index - 2
  while i > 0 and input:find('^[ \t]', i) do i = i - 1 end
  if i < 1 or input:find('^[\r\n;]', i) then return index end
end) * l.nonnewline^0)

M._rules = {
  {'whitespace', whitespace},
  {'comment', comment},
  {'separator', separator},
  {'expander', expander},
  {'braces', braces},
  {'quotes', quotes},
  {'brackets', brackets},
  {'variable', variable},
  {'backslash', backslash},
}

M._foldsymbols = {
  _patterns = {'[{}]', '#'},
  [l.KEYWORD] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
