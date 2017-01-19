-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- CoffeeScript LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, S = lpeg.P, lpeg.S

local M = {_NAME = 'coffeescript'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local block_comment = '###' * (l.any - '###')^0 * P('###')^-1
local line_comment = '#' * l.nonnewline_esc^0
local comment = token(l.COMMENT, block_comment + line_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local regex_str = #P('/') * l.last_char_includes('+-*%<>!=^&|?~:;,([{') *
                  l.delimited_range('/', true) * S('igm')^0
local string = token(l.STRING, sq_str + dq_str) + token(l.REGEX, regex_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'all', 'and', 'bind', 'break', 'by', 'case', 'catch', 'class', 'const',
  'continue', 'default', 'delete', 'do', 'each', 'else', 'enum', 'export',
  'extends', 'false', 'for', 'finally', 'function', 'if', 'import', 'in',
  'instanceof', 'is', 'isnt', 'let', 'loop', 'native', 'new', 'no', 'not', 'of',
  'off', 'on', 'or', 'return', 'super', 'switch', 'then', 'this', 'throw',
  'true', 'try', 'typeof', 'unless', 'until', 'var', 'void', 'with', 'when',
  'while', 'yes'
})

-- Fields: object properties and methods.
local field = token(l.FUNCTION, '.' * (S('_$') + l.alpha) *
                                (S('_$') + l.alnum)^0)

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'field', field},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'string', string},
  {'operator', operator},
}

M._FOLDBYINDENTATION = true

return M
