-- Copyright 2017-2020 Murray Calavera. See LICENSE.
-- Standard ML LPeg lexer.

local lexer = require('lexer')
local token = lexer.token

local function mlword(words)
  return lexer.word_match(words, "'")
end

local ws = token(lexer.WHITESPACE, lexer.space^1)

-- single line comments are valid in successor ml
local line_comment = lexer.to_eol('(*)')
local block_comment = lexer.range('(*', '*)', false, false, true)
local comment = token(lexer.COMMENT, line_comment + block_comment)

local string = token(lexer.STRING, lpeg.P('#')^-1 * lexer.range('"', true))

local function num(digit)
  return digit * (digit^0 * lpeg.P('_'))^0 * digit^1 + digit
end

local int = num(lexer.digit)
local frac = lpeg.P('.') * int
local minus = lpeg.P('~')^-1
local exp = lpeg.S('eE') * minus * int
local real = int * frac^-1 * exp + int * frac * exp^-1
local hex = num(lexer.xdigit)
local bin = num(lpeg.S('01'))

local number = token(lexer.NUMBER, lpeg.P('0w') * int +
  (lpeg.P('0wx') + lpeg.P('0xw')) * hex +
  (lpeg.P('0wb') + lpeg.P('0bw')) * bin + minus * lpeg.P('0x') * hex +
  minus * lpeg.P('0b') * bin + minus * real + minus * int)

local keyword = token(lexer.KEYWORD, mlword{
  'abstype', 'and', 'andalso', 'as', 'case', 'do', 'datatype', 'else', 'end',
  'exception', 'fn', 'fun', 'handle', 'if', 'in', 'infix', 'infixr', 'let',
  'local', 'nonfix', 'of', 'op', 'orelse', 'raise', 'rec', 'then',
  'type', 'val', 'with', 'withtype', 'while',

  'eqtype', 'functor', 'include', 'sharing', 'sig', 'signature',
  'struct', 'structure'
})

-- includes valid symbols for identifiers
local operator = token(lexer.OPERATOR,
  lpeg.S('!*/+-^:@=<>()[]{},;._|#%&$?~`\\'))

local type = token(lexer.TYPE, mlword{
  'int', 'real', 'word', 'bool', 'char', 'string', 'unit',
  'array', 'exn', 'list', 'option', 'order', 'ref', 'substring', 'vector'
})

-- `real`, `vector` and `substring` are a problem
local func = token(lexer.FUNCTION, mlword{
  'app', 'before', 'ceil', 'chr', 'concat', 'exnMessage', 'exnName',
  'explode', 'floor', 'foldl', 'foldr', 'getOpt', 'hd', 'ignore',
  'implode', 'isSome', 'length', 'map', 'not', 'null', 'ord', 'print',
  'real', 'rev', 'round', 'size', 'str', 'substring', 'tl', 'trunc',
  'valOf', 'vector',
  'o', 'abs', 'mod', 'div'
})

-- non-symbolic identifiers only
local id = (lexer.alnum + "'" + '_')^0
local aid = lexer.alpha * id
local longid = (aid * lpeg.P('.'))^0 * aid
local identifier = token(lexer.IDENTIFIER, lexer.lower * id)
local typevar = token(lexer.VARIABLE, "'" * id)
local c = mlword{'true', 'false', 'nil'}
local const = token(lexer.CONSTANT, lexer.upper * id + c)
local structure = token(lexer.CLASS, aid * lpeg.P('.'))

local open = token(lexer.KEYWORD, mlword{'open', 'structure', 'functor'}) * ws *
  token(lexer.CLASS, longid)

local struct_dec = token(lexer.KEYWORD, lpeg.P('structure')) * ws *
  token(lexer.CLASS, aid) * ws * token(lexer.OPERATOR, lpeg.P('=')) * ws

local struct_new = struct_dec * token(lexer.KEYWORD, lpeg.P('struct'))
local struct_alias = struct_dec * token(lexer.CLASS, longid)

local M = {_NAME = 'sml'}

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'number', number},
  {'struct_new', struct_new},
  {'struct_alias', struct_alias},
  {'structure', structure},
  {'open', open},
  {'type', type},
  {'keyword', keyword},
  {'function', func},
  {'string', string},
  {'operator', operator},
  {'typevar', typevar},
  {'constant', const},
  {'identifier', identifier},
}

return M
