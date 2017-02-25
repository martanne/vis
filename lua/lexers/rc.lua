-- Copyright 2017 Michael Forney. See LICENSE.
-- rc LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'rc'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local str = l.delimited_range("'", false, true)
local heredoc = '<<' * P(function(input, index)
  local s, e, _, delimiter =
    input:find('[ \t]*(["\']?)([%w!"%%+,-./:?@_~]+)%1', index)
  if s == index and delimiter then
    delimiter = delimiter:gsub('[%%+-.?]', '%%%1')
    local _, e = input:find('[\n\r]'..delimiter..'[\n\r]', e)
    return e and e + 1 or #input + 1
  end
end)
local string = token(l.STRING, str + heredoc)

-- Numbers.
local number = token(l.NUMBER, l.integer + l.float)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'for', 'in', 'while', 'if', 'not', 'switch', 'fn',
  'builtin', 'cd', 'eval', 'exec', 'exit', 'flag', 'rfork', 'shift', 'ulimit',
  'umask', 'wait', 'whatis', '.', '~',
}, '!"%*+,-./:?@[]~'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local variable = token(l.VARIABLE,
                       '$' * S('"#')^-1 * ('*' + l.digit^1 + l.word))

-- Operators.
local operator = token(l.OPERATOR, S('@`=!<>*&^|;?()[]{}') + '\\\n')

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'variable', variable},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '#'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
