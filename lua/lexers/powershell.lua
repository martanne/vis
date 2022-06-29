-- Copyright 2015-2022 Mitchell. See LICENSE.
-- PowerShell LPeg lexer.
-- Contributed by Jeff Stone.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('powershell')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'Begin', 'Break', 'Continue', 'Do', 'Else', 'End', 'Exit', 'For', 'ForEach', 'ForEach-Object',
  'Get-Date', 'Get-Random', 'If', 'Param', 'Pause', 'Powershell', 'Process', 'Read-Host', 'Return',
  'Switch', 'While', 'Write-Host'
}, true)))

-- Comparison Operators.
lex:add_rule('comparison', token(lexer.KEYWORD, '-' * word_match({
  'and', 'as', 'band', 'bor', 'contains', 'eq', 'ge', 'gt', 'is', 'isnot', 'le', 'like', 'lt',
  'match', 'ne', 'nomatch', 'not', 'notcontains', 'notlike', 'or', 'replace'
}, true)))

-- Parameters.
lex:add_rule('parameter', token(lexer.KEYWORD, '-' *
  word_match('Confirm Debug ErrorAction ErrorVariable OutBuffer OutVariable Verbose WhatIf', true)))

-- Properties.
lex:add_rule('property', token(lexer.KEYWORD, '.' *
  word_match('day dayofweek dayofyear hour millisecond minute month second timeofday year', true)))

-- Types.
lex:add_rule('type', token(lexer.KEYWORD, '[' * word_match({
  'array', 'boolean', 'byte', 'char', 'datetime', 'decimal', 'double', 'hashtable', 'int', 'long',
  'single', 'string', 'xml'
}, true) * ']'))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE,
  '$' * (lexer.digit^1 + lexer.word + lexer.range('{', '}', true))))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=!<>+-/*^&|~.,:;?()[]{}%`')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')

return lex
