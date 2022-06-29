-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Postscript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ps')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'pop', 'exch', 'dup', 'copy', 'roll', 'clear', 'count', 'mark', 'cleartomark', 'counttomark',
  'exec', 'if', 'ifelse', 'for', 'repeat', 'loop', 'exit', 'stop', 'stopped', 'countexecstack',
  'execstack', 'quit', 'start', 'true', 'false', 'NULL'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'add', 'div', 'idiv', 'mod', 'mul', 'sub', 'abs', 'ned', 'ceiling', 'floor', 'round', 'truncate',
  'sqrt', 'atan', 'cos', 'sin', 'exp', 'ln', 'log', 'rand', 'srand', 'rrand'
}))

-- Identifiers.
local word = (lexer.alpha + '-') * (lexer.alnum + '-')^0
lex:add_rule('identifier', token(lexer.IDENTIFIER, word))

-- Strings.
local arrow_string = lexer.range('<', '>')
local nested_string = lexer.range('(', ')', false, false, true)
lex:add_rule('string', token(lexer.STRING, arrow_string + nested_string))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('%')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Labels.
lex:add_rule('label', token(lexer.LABEL, '/' * word))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('[]{}')))

return lex
