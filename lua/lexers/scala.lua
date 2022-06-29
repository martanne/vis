-- Copyright 2006-2022 JMS. See LICENSE.
-- Scala LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('scala')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Classes.
lex:add_rule('class', token(lexer.KEYWORD, 'class') * ws^1 * token(lexer.CLASS, lexer.word))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abstract', 'case', 'catch', 'class', 'def', 'do', 'else', 'extends', 'false', 'final', 'finally',
  'for', 'forSome', 'if', 'implicit', 'import', 'lazy', 'match', 'new', 'null', 'object',
  'override', 'package', 'private', 'protected', 'return', 'sealed', 'super', 'this', 'throw',
  'trait', 'try', 'true', 'type', 'val', 'var', 'while', 'with', 'yield'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'Array', 'Boolean', 'Buffer', 'Byte', 'Char', 'Collection', 'Double', 'Float', 'Int', 'Iterator',
  'LinkedList', 'List', 'Long', 'Map', 'None', 'Option', 'Set', 'Short', 'SortedMap', 'SortedSet',
  'String', 'TreeMap', 'TreeSet'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, lexer.word) * #P('('))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local symbol = "'" * lexer.word
local dq_str = lexer.range('"', true)
local tq_str = lexer.range('"""')
lex:add_rule('string', token(lexer.STRING, tq_str + symbol + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('LlFfDd')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
