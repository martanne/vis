-- Copyright 2015-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- PowerShell LPeg lexer.
-- Contributed by Jeff Stone.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'powershell'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'Begin', 'Break', 'Continue', 'Do', 'Else', 'End', 'Exit', 'For', 'ForEach',
  'ForEach-Object', 'Get-Date', 'Get-Random', 'If', 'Param', 'Pause',
  'Powershell', 'Process', 'Read-Host', 'Return', 'Switch', 'While',
  'Write-Host'
}, '-', true))

-- Comparison Operators.
local comparison = token(l.KEYWORD, '-' * word_match({
  'and', 'as', 'band', 'bor', 'contains', 'eq', 'ge', 'gt', 'is', 'isnot', 'le',
  'like', 'lt', 'match', 'ne', 'nomatch', 'not', 'notcontains', 'notlike', 'or',
  'replace'
}, nil, true))

-- Parameters.
local parameter = token(l.KEYWORD, '-' * word_match({
  'Confirm', 'Debug', 'ErrorAction', 'ErrorVariable', 'OutBuffer',
  'OutVariable', 'Verbose', 'WhatIf'
}, nil, true))

-- Properties.
local property = token(l.KEYWORD, '.' * word_match({
  'day', 'dayofweek', 'dayofyear', 'hour', 'millisecond', 'minute', 'month',
  'second', 'timeofday', 'year'
}, nil, true))

-- Types.
local type = token(l.KEYWORD, '[' * word_match({
  'array', 'boolean', 'byte', 'char', 'datetime', 'decimal', 'double',
  'hashtable', 'int', 'long', 'single', 'string', 'xml'
}, nil, true) * ']')

-- Variables.
local variable = token(l.VARIABLE, '$' * (l.digit^1 + l.word +
                                          l.delimited_range('{}', true, true)))

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Operators.
local operator = token(l.OPERATOR, S('=!<>+-/*^&|~.,:;?()[]{}%`'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'keyword', keyword},
  {'comparison', comparison},
  {'parameter', parameter},
  {'property', property},
  {'type', type},
  {'variable', variable},
  {'string', string},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1}
}

return M
