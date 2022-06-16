-- Copyright 2018-2022 Simeon Maryasin (MarSoft). See LICENSE.
-- Fantom LPeg lexer.
-- Based on Java LPeg lexer by Mitchell and Vim's Fantom syntax.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('fantom')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Classes.
local type = token(lexer.TYPE, lexer.word)
lex:add_rule('class_sequence',
  token(lexer.KEYWORD, 'class') * ws * type * ( -- at most one inheritance spec
  ws * token(lexer.OPERATOR, ':') * ws * type *
    ( -- at least 0 (i.e. any number) of additional classes
    ws^-1 * token(lexer.OPERATOR, ',') * ws^-1 * type)^0)^-1)

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'using', 'native', -- external
  'goto', 'void', 'serializable', 'volatile', -- error
  'if', 'else', 'switch', -- conditional
  'do', 'while', 'for', 'foreach', 'each', -- repeat
  'true', 'false', -- boolean
  'null', -- constant
  'this', 'super', -- typedef
  'new', 'is', 'isnot', 'as', -- operator
  'plus', 'minus', 'mult', 'div', 'mod', 'get', 'set', 'slice', 'lshift', 'rshift', 'and', 'or',
  'xor', 'inverse', 'negate', --
  'increment', 'decrement', 'equals', 'compare', -- long operator
  'return', -- stmt
  'static', 'const', 'final', -- storage class
  'virtual', 'override', 'once', -- slot
  'readonly', -- field
  'throw', 'try', 'catch', 'finally', -- exceptions
  'assert', -- assert
  'class', 'enum', 'mixin', -- typedef
  'break', 'continue', -- branch
  'default', 'case', -- labels
  'public', 'internal', 'protected', 'private', 'abstract' -- scope decl
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match(
  'Void Bool Int Float Decimal Str Duration Uri Type Range List Map Obj Err Env')))

-- Functions.
-- lex:add_rule('function', token(lexer.FUNCTION, lexer.word) * #P('('))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local bq_str = lexer.range('`', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + bq_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('LlFfDd')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}#')))

-- Annotations.
lex:add_rule('facet', token('facet', '@' * lexer.word))
lex:add_style('facet', lexer.styles.preprocessor)

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
