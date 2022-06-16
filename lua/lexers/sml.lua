-- Copyright 2017-2022 Murray Calavera. See LICENSE.
-- Standard ML LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('sml')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Structures.
local id = (lexer.alnum + "'" + '_')^0
local aid = lexer.alpha * id
local longid = (aid * '.')^0 * aid
local struct_dec = token(lexer.KEYWORD, 'structure') * ws * token(lexer.CLASS, aid) * ws *
  token(lexer.OPERATOR, '=') * ws
lex:add_rule('struct_new', struct_dec * token(lexer.KEYWORD, 'struct'))
lex:add_rule('struct_alias', struct_dec * token(lexer.CLASS, longid))
lex:add_rule('structure', token(lexer.CLASS, aid * '.'))

-- Open.
lex:add_rule('open', token(lexer.KEYWORD, word_match('open structure functor')) * ws *
  token(lexer.CLASS, longid))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abstype', 'and', 'andalso', 'as', 'case', 'do', 'datatype', 'else', 'end', 'exception', 'fn',
  'fun', 'handle', 'if', 'in', 'infix', 'infixr', 'let', 'local', 'nonfix', 'of', 'op', 'orelse',
  'raise', 'rec', 'then', 'type', 'val', 'with', 'withtype', 'while', --
  'eqtype', 'functor', 'include', 'sharing', 'sig', 'signature', 'struct', 'structure'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'int', 'real', 'word', 'bool', 'char', 'string', 'unit', 'array', 'exn', 'list', 'option',
  'order', 'ref', 'substring', 'vector'
}))

-- Functions.
-- `real`, `vector` and `substring` are a problem.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'app', 'before', 'ceil', 'chr', 'concat', 'exnMessage', 'exnName', 'explode', 'floor', 'foldl',
  'foldr', 'getOpt', 'hd', 'ignore', 'implode', 'isSome', 'length', 'map', 'not', 'null', 'ord',
  'print', 'real', 'rev', 'round', 'size', 'str', 'substring', 'tl', 'trunc', 'valOf', 'vector',
  'o', 'abs', 'mod', 'div'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match('true false nil') + lexer.upper * id))

-- Indentifiers (non-symbolic).
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.lower * id))

-- Strings.
lex:add_rule('string', token(lexer.STRING, P('#')^-1 * lexer.range('"', true)))

-- Comments.
local line_comment = lexer.to_eol('(*)')
local block_comment = lexer.range('(*', '*)', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
local function num(digit) return digit * (digit^0 * '_')^0 * digit^1 + digit end
local int = num(lexer.digit)
local frac = '.' * int
local minus = lpeg.P('~')^-1
local exp = lpeg.S('eE') * minus * int
local real = int * frac^-1 * exp + int * frac * exp^-1
local hex = num(lexer.xdigit)
local bin = num(lpeg.S('01'))
-- LuaFormatter off
lex:add_rule('number', token(lexer.NUMBER,
  '0w' * int +
  (P('0wx') + '0xw') * hex +
  (P('0wb') + '0bw') * bin +
  minus * '0x' * hex +
  minus * '0b' * bin +
  minus * real +
  minus * int))
-- LuaFormatter on

-- Type variables.
lex:add_rule('typevar', token(lexer.VARIABLE, "'" * id))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!*/+-^:@=<>()[]{},;._|#%&$?~`\\')))

return lex
