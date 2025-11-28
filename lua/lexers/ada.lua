-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Ada LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ada')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
	'abort', 'abs', 'abstract', 'accept', 'access', 'aliased', 'all', 'and', 'array', 'at', 'begin',
	'body', 'case', 'constant', 'declare', 'delay', 'delta', 'digits', 'do', 'else', 'elsif', 'end',
	'entry', 'exception', 'exit', 'for', 'function', 'generic', 'goto', 'if', 'in', 'interface', 'is',
	'limited', 'loop', 'mod', 'new', 'not', 'null', 'of', 'or', 'others', 'out', 'overriding',
	'package', 'parallel', 'pragma', 'private', 'procedure', 'protected', 'raise', 'range', 'record',
	'rem', 'renames', 'requeue', 'return', 'reverse', 'select', 'separate', 'some', 'subtype',
	'synchronized', 'tagged', 'task', 'terminate', 'then', 'type', 'until', 'use', 'when', 'while',
	'with', 'xor', --
	'true', 'false'
}, true)))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match({
	'boolean', 'character', 'count', 'duration', 'float', 'integer', 'long_float', 'long_integer',
	'priority', 'short_float', 'short_integer', 'string'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true, false)))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('--')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number_('_')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, '..' + S(':;=<>&+-*/.()')))

lexer.property['scintillua.comment'] = '--'

return lex
