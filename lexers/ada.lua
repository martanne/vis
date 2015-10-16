-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Ada LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'ada'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '--' * l.nonnewline^0)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true, true))

-- Numbers.
local hex_num = 'O' * S('xX') * (l.xdigit + '_')^1
local integer = l.digit^1 * ('_' * l.digit^1)^0
local float = integer^1 * ('.' * integer^0)^-1 * S('eE') * S('+-')^-1 * integer
local number = token(l.NUMBER, hex_num + S('+-')^-1 * (float + integer) *
                               S('LlUuFf')^-3)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'abort', 'abs', 'accept', 'all', 'and', 'begin', 'body', 'case', 'declare',
  'delay', 'do', 'else', 'elsif', 'end', 'entry', 'exception', 'exit', 'for',
  'generic', 'goto', 'if', 'in', 'is', 'loop', 'mod', 'new', 'not', 'null',
  'or', 'others', 'out', 'protected', 'raise', 'record', 'rem', 'renames',
  'requeue', 'reverse', 'select', 'separate', 'subtype', 'task', 'terminate',
  'then', 'type', 'until', 'when', 'while', 'xor',
  -- Preprocessor.
  'package', 'pragma', 'use', 'with',
  -- Function
  'function', 'procedure', 'return',
  -- Storage class.
  'abstract', 'access', 'aliased', 'array', 'at', 'constant', 'delta', 'digits',
  'interface', 'limited', 'of', 'private', 'range', 'tagged', 'synchronized',
  -- Boolean.
  'true', 'false'
})

-- Types.
local type = token(l.TYPE, word_match{
  'boolean', 'character', 'count', 'duration', 'float', 'integer', 'long_float',
  'long_integer', 'priority', 'short_float', 'short_integer', 'string'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S(':;=<>&+-*/.()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
