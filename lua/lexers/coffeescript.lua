-- Copyright 2006-2022 Mitchell. See LICENSE.
-- CoffeeScript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('coffeescript', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'all', 'and', 'bind', 'break', 'by', 'case', 'catch', 'class', 'const', 'continue', 'default',
  'delete', 'do', 'each', 'else', 'enum', 'export', 'extends', 'false', 'finally', 'for',
  'function', 'if', 'import', 'in', 'instanceof', 'is', 'isnt', 'let', 'loop', 'native', 'new',
  'no', 'not', 'of', 'off', 'on', 'or', 'return', 'super', 'switch', 'then', 'this', 'throw',
  'true', 'try', 'typeof', 'unless', 'until', 'var', 'void', 'when', 'while', 'with', 'yes'
}))

-- Fields: object properties and methods.
lex:add_rule('field',
  token(lexer.FUNCTION, '.' * (S('_$') + lexer.alpha) * (S('_$') + lexer.alnum)^0))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local string = token(lexer.STRING, sq_str + dq_str)
local regex_str =
  #P('/') * lexer.last_char_includes('+-*%<>!=^&|?~:;,([{') * lexer.range('/', true) * S('igm')^0
local regex = token(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Comments.
local block_comment = lexer.range('###')
local line_comment = lexer.to_eol('#', true)
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}')))

return lex
