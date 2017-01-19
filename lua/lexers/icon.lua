-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- LPeg lexer for the Icon programming language.
-- http://www.cs.arizona.edu/icon
-- Contributed by Carl Sturtivant.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'icon'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

--Comments
local line_comment = '#' * l.nonnewline_esc^0
local comment = token(l.COMMENT, line_comment)

-- Strings.
local cset = l.delimited_range("'")
local str = l.delimited_range('"')
local string = token(l.STRING, cset + str)

-- Numbers.
local radix_literal = P('-')^-1 * l.dec_num * S('rR') * l.alnum^1
local number = token(l.NUMBER, radix_literal + l.float + l.integer)

-- Preprocessor.
local preproc_word = word_match{
  'include', 'line', 'define', 'undef', 'ifdef', 'ifndef', 'else', 'endif',
  'error'
}
local preproc = token(l.PREPROCESSOR, S(' \t')^0 * P('$') * preproc_word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'break', 'by', 'case', 'create', 'default', 'do', 'else', 'end', 'every',
  'fail', 'global', 'if', 'initial', 'invocable', 'link', 'local', 'next',
  'not', 'of', 'procedure', 'record', 'repeat', 'return', 'static', 'suspend',
  'then', 'to', 'until', 'while'
})

-- Icon Keywords: unique to Icon; use l.TYPE, as Icon is dynamically typed
local type = token(l.TYPE, P('&') * word_match{
  'allocated', 'ascii', 'clock', 'collections', 'cset', 'current', 'date',
  'dateline', 'digits', 'dump', 'e', 'error', 'errornumber', 'errortext',
  'errorvalue', 'errout', 'fail', 'features', 'file', 'host', 'input', 'lcase',
  'letters', 'level', 'line', 'main', 'null', 'output', 'phi', 'pi', 'pos',
  'progname', 'random', 'regions', 'source', 'storage', 'subject', 'time',
  'trace', 'ucase', 'version'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>~!=^&|?~@:;,.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'identifier', identifier},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'%l+', '#'},
  [l.PREPROCESSOR] = {ifdef = 1, ifndef = 1, endif = -1},
  [l.KEYWORD] = { procedure = 1, ['end'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
