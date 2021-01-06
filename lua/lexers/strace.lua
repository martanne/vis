-- Copyright 2017-2021 Marc André Tanner. See LICENSE.
-- strace(1) output lexer

local l = require('lexer')
local token, word_match = l.token, l.word_match
local S, B = lpeg.S, lpeg.B

local M = {_NAME = 'strace'}

local ws = token(l.WHITESPACE, l.space^1)
local string = token(l.STRING, l.delimited_range('"', true) + l.delimited_range("'", true))
local number = token(l.NUMBER, l.float + l.integer)
local constant = token(l.CONSTANT, (l.upper + '_') * (l.upper + l.digit + '_')^0)
local syscall = token(l.KEYWORD, l.starts_line(l.word))
local operator = token(l.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}'))
local comment = token(l.COMMENT, l.nested_pair('/*', '*/') + (l.delimited_range('()') * l.newline))
local result = token(l.TYPE, B(' = ') * l.integer)
local identifier = token(l.IDENTIFIER, l.word)

M._rules = {
  {'whitespace', ws},
  {'syscall', syscall},
  {'constant', constant},
  {'string', string},
  {'comment', comment},
  {'result', result},
  {'identifier', identifier},
  {'number', number},
  {'operator', operator},
}

M._LEXBYLINE = true

return M
