-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Gap LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'gap'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.digit^1 * -l.alpha)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'and', 'break', 'continue', 'do', 'elif', 'else', 'end', 'fail', 'false',
  'fi', 'for', 'function', 'if', 'in', 'infinity', 'local', 'not', 'od', 'or',
  'rec', 'repeat', 'return', 'then', 'true', 'until', 'while'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('*+-,./:;<=>~^#()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '#'},
  [l.KEYWORD] = {
    ['function'] = 1, ['end'] = -1, ['do'] = 1, od = -1, ['if'] = 1, fi = -1,
    ['repeat'] = 1, ['until'] = -1
  },
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
