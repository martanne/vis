-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Ada LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ada')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abort', 'abs', 'accept', 'all', 'and', 'begin', 'body', 'case', 'declare', 'delay', 'do', 'else',
  'elsif', 'end', 'entry', 'exception', 'exit', 'for', 'generic', 'goto', 'if', 'in', 'is', 'loop',
  'mod', 'new', 'not', 'null', 'or', 'others', 'out', 'protected', 'raise', 'record', 'rem',
  'renames', 'requeue', 'reverse', 'select', 'separate', 'subtype', 'task', 'terminate', 'then',
  'type', 'until', 'when', 'while', 'xor',
  -- Preprocessor.
  'package', 'pragma', 'use', 'with',
  -- Function.
  'function', 'procedure', 'return',
  -- Storage class.
  'abstract', 'access', 'aliased', 'array', 'at', 'constant', 'delta', 'digits', 'interface',
  'limited', 'of', 'private', 'range', 'tagged', 'synchronized',
  -- Boolean.
  'true', 'false'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'boolean', 'character', 'count', 'duration', 'float', 'integer', 'long_float', 'long_integer',
  'priority', 'short_float', 'short_integer', 'string'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true, false)))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('--')))

-- Numbers.
local integer = lexer.digit^1 * ('_' * lexer.digit^1)^0
local float = integer^1 * ('.' * integer^0)^-1 * S('eE') * S('+-')^-1 * integer
lex:add_rule('number', token(lexer.NUMBER, S('+-')^-1 * (float + integer)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S(':;=<>&+-*/.()')))

return lex
