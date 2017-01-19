-- Copyright 2015-2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- Faust LPeg lexer, see http://faust.grame.fr/

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'faust'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Numbers.
local int = R('09')^1
local rad = P('.')
local exp = (P('e') * S('+-')^-1 * int)^-1
local flt = int * (rad * int)^-1 * exp + int^-1 * rad * int * exp
local number = token(l.NUMBER, flt + int)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'declare', 'import', 'mdoctags', 'dependencies', 'distributed', 'inputs',
  'outputs', 'par', 'seq', 'sum', 'prod', 'xor', 'with', 'environment',
  'library', 'component', 'ffunction', 'fvariable', 'fconstant', 'int', 'float',
  'case', 'waveform', 'h:', 'v:', 't:'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local punct = S('+-/*%<>~!=^&|?~:;,.()[]{}@#$`\\\'')
local operator = token(l.OPERATOR, punct)

-- Pragmas.
local mdoc = P('<mdoc>') * (l.any - P('</mdoc>'))^0 * P('</mdoc>')
local pragma = token(l.PREPROCESSOR, mdoc)

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
