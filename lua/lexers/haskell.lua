-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Haskell LPeg lexer.
-- Modified by Alex Suraci.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('haskell', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'case', 'class', 'data', 'default', 'deriving', 'do', 'else', 'if', 'import', 'in', 'infix',
  'infixl', 'infixr', 'instance', 'let', 'module', 'newtype', 'of', 'then', 'type', 'where', '_',
  'as', 'qualified', 'hiding'
}))

-- Types & type constructors.
local word = (lexer.alnum + S("._'#"))^0
local op = lexer.punct - S('()[]{}')
lex:add_rule('type', token(lexer.TYPE, (lexer.upper * word) + (':' * (op^1 - ':'))))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '_') * word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('--', true)
local block_comment = lexer.range('{-', '-}')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, op))

return lex
