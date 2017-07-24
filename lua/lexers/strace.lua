-- Copyright 2017 Marc André Tanner. See LICENSE.
-- strace(1) output lexer

local l = require('lexer')
local token, word_match = l.token, l.word_match
local S = lpeg.S

local M = {_NAME = 'strace'}

local ws = token(l.WHITESPACE, l.space^1)
local string = token(l.STRING, l.delimited_range('"', true) + l.delimited_range("'", true))
local number = token(l.NUMBER, l.float + l.integer)
local constant = token(l.CONSTANT, (l.upper + '_') * (l.upper + l.digit + '_')^0)
local syscall = token(l.KEYWORD, l.starts_line(l.word))
local operator = token(l.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}'))
local comment = token(l.COMMENT, l.nested_pair('/*', '*/') + ('(' * (l.alpha + ' ')^1 * ')\n'))
local result = token(l.TYPE, '= ' * l.integer)

M._rules = {
  {'whitespace', ws},
  {'keyword', syscall},
  {'constant', constant},
  {'string', string},
  {'comment', comment},
  {'type', result},
  {'number', number},
  {'operator', operator},
}

M._LEXBYLINE = true

return M
