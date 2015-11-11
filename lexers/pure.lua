-- pure LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'pure'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = P('L')^-1 * l.delimited_range("'", true)
local dq_str = P('L')^-1 * l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local bin = '0' * S('Bb') * S('01')^1
local hex = '0' * S('Xx') * (R('09') + R('af') + R('AF'))^1
local int = R('09')^1
local exp = S('Ee') * S('+-')^-1 * int
local flt = int * ('.' * int)^-1 * exp + int^-1 * '.' * int * exp^-1
local number = token(l.NUMBER, flt + bin + hex + int * P('L')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'namespace', 'with', 'end', 'using', 'interface', 'extern', 'let',
  'const', 'def', 'type', 'public', 'private', 'nonfix', 'outfix',
  'infix', 'infixl', 'infixr', 'prefix', 'postfix', 'if', 'otherwise',
  'when', 'case', 'of', 'then', 'else'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}@\\'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'number', number},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'operator', operator},
}

return M
