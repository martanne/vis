-- Copyright 2015-2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- pure LPeg lexer, see http://purelang.bitbucket.org/

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'pure'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Numbers.
local bin = '0' * S('Bb') * S('01')^1
local hex = '0' * S('Xx') * (R('09') + R('af') + R('AF'))^1
local dec = R('09')^1
local int = (bin + hex + dec) * P('L')^-1
local rad = P('.') - P('..')
local exp = (S('Ee') * S('+-')^-1 * int)^-1
local flt = int * (rad * dec)^-1 * exp + int^-1 * rad * dec * exp
local number = token(l.NUMBER, flt + int)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'namespace', 'with', 'end', 'using', 'interface', 'extern', 'let', 'const',
  'def', 'type', 'public', 'private', 'nonfix', 'outfix', 'infix', 'infixl',
  'infixr', 'prefix', 'postfix', 'if', 'otherwise', 'when', 'case', 'of',
  'then', 'else'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local punct = S('+-/*%<>~!=^&|?~:;,.()[]{}@#$`\\\'')
local dots = P('..')
local operator = token(l.OPERATOR, dots + punct)

-- Pragmas.
local hashbang = l.starts_line('#!') * (l.nonnewline - P('//'))^0
local pragma = token(l.PREPROCESSOR, hashbang)

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'pragma', pragma},
  {'keyword', keyword},
  {'number', number},
  {'operator', operator},
  {'identifier', identifier},
  {'string', string},
}

return M
