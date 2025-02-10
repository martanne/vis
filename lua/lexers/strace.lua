-- Copyright 2017-2025 Marc André Tanner. See LICENSE.
-- strace(1) output lexer

local lexer = lexer
local S, B = lpeg.S, lpeg.B

local lex = lexer.new(..., {lex_by_line = true})

-- Syscall
lex:add_rule('syscall', lex:tag(lexer.FUNCTION, lexer.starts_line(lexer.word)))

-- Upper case constants
lex:add_rule('constant',
	lex:tag(lexer.CONSTANT, (lexer.upper + '_') * (lexer.upper + lexer.digit + '_')^0))

-- Single and double quoted strings
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Comments and text in parentheses at the line end
local comment = lexer.range('/*', '*/')
local description = lexer.range('(', ')') * lexer.newline
lex:add_rule('comment', lex:tag(lexer.COMMENT, comment + description))

lex:add_rule('result', lex:tag(lexer.TYPE, B(' = ') * lexer.integer))
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.float + lexer.integer))
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}')))

return lex
