-- Copyright 2006-2025 Mitchell. See LICENSE.
-- CoffeeScript LPeg lexer.

local lexer = require('lexer')
local word_match = lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('coffeescript', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', lex:tag(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, word_match{
	'all', 'and', 'bind', 'break', 'by', 'case', 'catch', 'class', 'const', 'continue', 'default',
	'delete', 'do', 'each', 'else', 'enum', 'export', 'extends', 'false', 'finally', 'for',
	'function', 'if', 'import', 'in', 'instanceof', 'is', 'isnt', 'let', 'loop', 'native', 'new',
	'no', 'not', 'of', 'off', 'on', 'or', 'return', 'super', 'switch', 'then', 'this', 'throw',
	'true', 'try', 'typeof', 'unless', 'until', 'var', 'void', 'when', 'while', 'with', 'yes'
}))

-- Fields: object properties and methods.
lex:add_rule('field',
	lex:tag(lexer.FUNCTION, '.' * (S('_$') + lexer.alpha) * (S('_$') + lexer.alnum)^0))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local string = lex:tag(lexer.STRING, sq_str + dq_str)
local regex_str = lexer.after_set('+-*%<>!=^&|?~:;,([{', lexer.range('/', true) * S('igm')^0)
local regex = lex:tag(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Comments.
local block_comment = lexer.range('###')
local line_comment = lexer.to_eol('#', true)
lex:add_rule('comment', lex:tag(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}')))

lexer.property['scintillua.comment'] = '#'

return lex
