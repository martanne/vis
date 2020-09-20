-- Copyright 2020 Christian Hesse
-- Mikrotik RouterOS script LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'routeros'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local string = token(l.STRING, l.delimited_range('"'))

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  -- control
  ':delay',
  ':do', 'on-error', 'while',
  ':error',
  ':foreach', 'in', 'do',
  ':for', 'from', 'to', 'step',
  ':if', 'do', 'else',
  ':return',
  ':while', 'do',
  -- menu specific commands
  'add',
  'disable',
  'edit',
  'enable',
  'export',
  'find',
  'get',
  'info',
  'monitor',
  'print', 'append', 'as-value', 'brief', 'count-only', 'detail', 'file',
      'follow', 'follow-only', 'from', 'interval', 'terse', 'value-list',
      'where', 'without-paging',
  'remove',
  'set',
  -- output & string handling
  ':beep',
  ':blink',
  ':environment',
  ':execute',
  ':find',
  ':len',
  ':log', 'alert', 'critical', 'debug', 'emergency', 'error', 'info',
      'notice', 'warning',
  ':parse',
  ':pick',
  ':put',
  ':terminal',
  ':time',
  ':typeof',
  -- variable declaration
  ':global',
  ':local',
  ':set',
  -- variable casting
  ':toarray',
  ':tobool',
  ':toid',
  ':toip',
  ':toip6',
  ':tonum',
  ':tostr',
  ':totime',
  -- boolean values and logical operators
  'false', 'no',
  'true', 'yes',
  'and',
  'in',
  'or',
  -- networking
  ':ping',
  ':resolve'
}, ':-'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local variable = token(l.VARIABLE,
                       '$' * (S('!#?*@$') + l.digit^1 + l.word +
                              l.delimited_range('{}', true, true, true)))

-- Operators.
local operator = token(l.OPERATOR, S('=!%<>+-/*&|~.,;()[]{}'))

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
  _patterns = {'[a-z]+', '[{}]', '#'},
  [l.KEYWORD] = { },
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
