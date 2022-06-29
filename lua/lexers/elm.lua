-- Copyright 2020-2022 Mitchell. See LICENSE.
-- Elm LPeg lexer
-- Adapted from Haskell LPeg lexer by Karl Schultheisz.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('elm', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match(
  'if then else case of let in module import as exposing type alias port')))

-- Types & type constructors.
local word = (lexer.alnum + S("._'#"))^0
local op = lexer.punct - S('()[]{}')
lex:add_rule('type', token(lexer.TYPE, lexer.upper * word + ':' * (op^1 - ':')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '_') * word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"')))

-- Chars.
lex:add_rule('character', token(lexer.STRING, lexer.range("'", true)))

-- Comments.
local line_comment = lexer.to_eol('--', true)
local block_comment = lexer.range('{-', '-}', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, op))

return lex
