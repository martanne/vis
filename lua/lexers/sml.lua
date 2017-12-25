-- Copyright 2017 Murray Calavera. See LICENSE.
-- Standard ML LPeg lexer.

local l = require('lexer')
local token = l.token

local function mlword(words)
  return l.word_match(words, "'")
end

local ws = token(l.WHITESPACE, l.space^1)

-- single line comments are valid in successor ml
local cl = '(*)' * l.nonnewline^0
local comment = token(l.COMMENT, cl + l.nested_pair('(*', '*)'))

local string = token(l.STRING, lpeg.P('#')^-1 * l.delimited_range('"', true))

local function num(digit)
  return digit * (digit^0 * lpeg.P('_'))^0 * digit^1 + digit
end

local int = num(l.digit)
local frac = lpeg.P('.') * int
local minus = lpeg.P('~')^-1
local exp = lpeg.S('eE') * minus * int
local real = int * frac^-1 * exp + int * frac * exp^-1
local hex = num(l.xdigit)
local bin = num(lpeg.S('01'))

local number = token(l.NUMBER,
  lpeg.P('0w') * int
  + (lpeg.P('0wx') + lpeg.P('0xw')) * hex
  + (lpeg.P('0wb') + lpeg.P('0bw')) * bin
  + minus * lpeg.P('0x') * hex
  + minus * lpeg.P('0b') * bin
  + minus * real
  + minus * int
)

local keyword = token(l.KEYWORD, mlword{
  'abstype', 'and', 'andalso', 'as', 'case', 'do', 'datatype', 'else', 'end',
  'exception', 'fn', 'fun', 'handle', 'if', 'in', 'infix', 'infixr', 'let',
  'local', 'nonfix', 'of', 'op', 'orelse', 'raise', 'rec', 'then',
  'type', 'val', 'with', 'withtype', 'while',

  'eqtype', 'functor', 'include', 'sharing', 'sig', 'signature',
  'struct', 'structure'
})

-- includes valid symbols for identifiers
local operator = token(l.OPERATOR, lpeg.S('!*/+-^:@=<>()[]{},;._|#%&$?~`\\'))

local type = token(l.TYPE, mlword{
  'int', 'real', 'word', 'bool', 'char', 'string', 'unit',
  'array', 'exn', 'list', 'option', 'order', 'ref', 'substring', 'vector'
})

-- `real`, `vector` and `substring` are a problem
local func = token(l.FUNCTION, mlword{
  'app', 'before', 'ceil', 'chr', 'concat', 'exnMessage', 'exnName',
  'explode', 'floor', 'foldl', 'foldr', 'getOpt', 'hd', 'ignore',
  'implode', 'isSome', 'length', 'map', 'not', 'null', 'ord', 'print',
  'real', 'rev', 'round', 'size', 'str', 'substring', 'tl', 'trunc',
  'valOf', 'vector',
  'o', 'abs', 'mod', 'div'
})

-- non-symbolic identifiers only
local id = (l.alnum + "'" + '_')^0
local aid = l.alpha * id
local longid = (aid * lpeg.P('.'))^0 * aid
local identifier = token(l.IDENTIFIER, l.lower * id)
local typevar = token(l.VARIABLE, "'" * id)
local c = mlword{'true', 'false', 'nil'}
local const = token(l.CONSTANT, l.upper * id + c)
local structure = token(l.CLASS, aid * lpeg.P('.'))

local open
  = token(l.KEYWORD, mlword{'open', 'structure', 'functor'})
  * ws * token(l.CLASS, longid)

local struct_dec
  = token(l.KEYWORD, lpeg.P('structure')) * ws
  * token(l.CLASS, aid) * ws
  * token(l.OPERATOR, lpeg.P('=')) * ws

local struct_new = struct_dec * token(l.KEYWORD, lpeg.P('struct'))
local struct_alias = struct_dec * token(l.CLASS, longid)

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
