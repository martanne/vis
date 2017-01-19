-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Applescript LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'applescript'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '--' * l.nonnewline^0
local block_comment = '(*' * (l.any - '*)')^0 * P('*)')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'script', 'property', 'prop', 'end', 'copy', 'to', 'set', 'global', 'local',
  'on', 'to', 'of', 'in', 'given', 'with', 'without', 'return', 'continue',
  'tell', 'if', 'then', 'else', 'repeat', 'times', 'while', 'until', 'from',
  'exit', 'try', 'error', 'considering', 'ignoring', 'timeout', 'transaction',
  'my', 'get', 'put', 'into', 'is',
  -- References.
  'each', 'some', 'every', 'whose', 'where', 'id', 'index', 'first', 'second',
  'third', 'fourth', 'fifth', 'sixth', 'seventh', 'eighth', 'ninth', 'tenth',
  'last', 'front', 'back', 'st', 'nd', 'rd', 'th', 'middle', 'named', 'through',
  'thru', 'before', 'after', 'beginning', 'the',
  -- Commands.
  'close', 'copy', 'count', 'delete', 'duplicate', 'exists', 'launch', 'make',
  'move', 'open', 'print', 'quit', 'reopen', 'run', 'save', 'saving',
  -- Operators.
  'div', 'mod', 'and', 'not', 'or', 'as', 'contains', 'equal', 'equals',
  'isn\'t',
}, "'", true))

-- Constants.
local constant = token(l.CONSTANT, word_match({
  'case', 'diacriticals', 'expansion', 'hyphens', 'punctuation',
  -- Predefined variables.
  'it', 'me', 'version', 'pi', 'result', 'space', 'tab', 'anything',
  -- Text styles.
  'bold', 'condensed', 'expanded', 'hidden', 'italic', 'outline', 'plain',
  'shadow', 'strikethrough', 'subscript', 'superscript', 'underline',
  -- Save options.
  'ask', 'no', 'yes',
  -- Booleans.
  'false', 'true',
  -- Date and time.
  'weekday', 'monday', 'mon', 'tuesday', 'tue', 'wednesday', 'wed', 'thursday',
  'thu', 'friday', 'fri', 'saturday', 'sat', 'sunday', 'sun', 'month',
  'january', 'jan', 'february', 'feb', 'march', 'mar', 'april', 'apr', 'may',
  'june', 'jun', 'july', 'jul', 'august', 'aug', 'september', 'sep', 'october',
  'oct', 'november', 'nov', 'december', 'dec', 'minutes', 'hours', 'days',
  'weeks'
}, nil, true))

-- Identifiers.
local identifier = token(l.IDENTIFIER, (l.alpha + '_') * l.alnum^0)

-- Operators.
local operator = token(l.OPERATOR, S('+-^*/&<>=:,(){}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'constant', constant},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
