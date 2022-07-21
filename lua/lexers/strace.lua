-- Copyright 2017-2021 Marc André Tanner. See LICENSE.
-- strace(1) output lexer

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local S, B = lpeg.S, lpeg.B

local lex = lexer.new('strace', {lex_by_line = true})

-- Whitespace
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Syscall
lex:add_rule('syscall', token(lexer.KEYWORD, lexer.starts_line(lexer.word)))

-- Upper case constants
lex:add_rule('constant', token(lexer.CONSTANT,
  (lexer.upper + '_') * (lexer.upper + lexer.digit + '_')^0))

-- Single and double quoted strings
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments and text in parentheses at the line end
local comment = lexer.range('/*', '*/')
local description = lexer.range('(', ')') * lexer.newline
lex:add_rule('comment', token(lexer.COMMENT, comment + description))

lex:add_rule('result', token(lexer.TYPE, B(' = ') * lexer.integer))
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))
lex:add_rule('number', token(lexer.NUMBER, lexer.float + lexer.integer))
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}')))

return lex
