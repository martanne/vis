-- Copyright 2020-2022 Christian Hesse. See LICENSE.
-- Mikrotik RouterOS script LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('routeros')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- Control.
  ':delay', ':do', 'on-error', 'while', ':error', ':foreach', 'in', 'do', ':for', 'from', 'to',
  'step', ':if', 'do', 'else', ':return', ':while', 'do',
  -- Menu specific commands.
  'add', 'disable', 'edit', 'enable', 'export', 'find', 'get', 'info', 'monitor', 'print', 'append',
  'as-value', 'brief', 'count-only', 'detail', 'file', 'follow', 'follow-only', 'from', 'interval',
  'terse', 'value-list', 'where', 'without-paging', 'remove', 'set',
  -- Output & string handling.
  ':beep', ':blink', ':environment', ':execute', ':find', ':len', ':log', 'alert', 'critical',
  'debug', 'emergency', 'error', 'info', 'notice', 'warning', ':parse', ':pick', ':put',
  ':terminal', ':time', ':typeof',
  -- Variable declaration.
  ':global', ':local', ':set',
  -- Variable casting.
  ':toarray', ':tobool', ':toid', ':toip', ':toip6', ':tonum', ':tostr', ':totime',
  -- Boolean values and logical operators.
  'false', 'no', 'true', 'yes', 'and', 'in', 'or',
  -- Networking.
  ':ping', ':resolve'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"')))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, '$' *
  (S('!#?*@$') + lexer.digit^1 + lexer.word + lexer.range('{', '}', true, false, true))))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=!%<>+-/*&|~.,;()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
