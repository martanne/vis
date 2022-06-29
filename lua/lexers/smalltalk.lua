-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Smalltalk LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('smalltalk')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match(
  'true false nil self super isNil not Smalltalk Transcript')))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match(
  'Date Time Boolean True False Character String Array Symbol Integer Object')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local word_str = '$' * lexer.word
lex:add_rule('string', token(lexer.STRING, sq_str + word_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.range('"', false, false)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S(':=_<>+-/*!()[]')))

-- Labels.
lex:add_rule('label', token(lexer.LABEL, '#' * lexer.word))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '[', ']')

return lex
