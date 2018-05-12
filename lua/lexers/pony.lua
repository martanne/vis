-- Copyright 2017 Murray Calavera. See LICENSE.
-- Pony LPeg lexer.

local l = require('lexer')
local token = l.token
local P, S = lpeg.P, lpeg.S

local function pword(words)
  return l.word_match(words, "'")
end

local ws = token(l.WHITESPACE, l.space^1)

local comment_line = '//' * l.nonnewline^0
local comment_block = l.nested_pair('/*', '*/')
local comment = token(l.COMMENT, comment_line + comment_block)

local annotation = token(l.PREPROCESSOR, l.delimited_range('\\', false, true))

local lit_bool = token(l.CONSTANT, pword{'true', 'false'})

local nq = l.any - P'"'
local lit_str = token(l.STRING,
  P'"""' * (nq + (P'"' * #(nq + (P'"' * nq))))^0 * P'"""'
  + l.delimited_range('"')
  + l.delimited_range("'")
)

local function num(digit)
  return digit * (digit^0 * P'_')^0 * digit^1 + digit
end

local int = num(l.digit)
local frac = P('.') * int
local exp = S('eE') * (P('-') + P('+'))^-1 * int

local lit_num = token(l.NUMBER,
  P('0x') * num(l.xdigit)
  + P('0b') * num(S('01'))
  + int * frac^-1 * exp^-1
)

local keyword = token(l.KEYWORD, pword{
  'actor', 'as', 'be', 'break', 'class', 'compile_error', 'compile_intrinsic',
  'continue', 'consume', 'do', 'else', 'elseif', 'embed', 'end', 'error',
  'for', 'fun', 'if', 'ifdef', 'iftype', 'in', 'interface', 'is', 'isnt',
  'lambda', 'let', 'match', 'new', 'object', 'primitive', 'recover', 'repeat',
  'return', 'struct', 'then', 'this', 'trait', 'try', 'type', 'until', 'use',
  'var', 'where', 'while', 'with'})
local capability = token(l.LABEL, pword{
  'box', 'iso', 'ref', 'tag', 'trn', 'val'})
local qualifier = token(l.LABEL,
  P'#' * pword{'read', 'send', 'share', 'any', 'alias'})

local operator = token(l.OPERATOR,
  pword{'and', 'or', 'xor', 'not', 'addressof', 'digestof'}
  + lpeg.Cmt(S('+-*/%<>=!~')^1, function(input, index, op)
      local ops = {
        ['+'] = true, ['-'] = true, ['*'] = true, ['/'] = true, ['%'] = true,
        ['+~'] = true, ['-~'] = true, ['*~'] = true, ['/~'] = true,
        ['%~'] = true, ['<<'] = true, ['>>'] = true, ['<<~'] = true,
        ['>>~'] = true, ['=='] = true, ['!='] = true, ['<'] = true,
        ['<='] = true, ['>='] = true, ['>'] = true, ['==~'] = true,
        ['!=~'] = true, ['<~'] = true, ['<=~'] = true, ['>=~'] = true,
        ['>~'] = true
      }
      return ops[op] and index or nil
    end)
)

-- there is no suitable token name for this, change this if ever one is added
local punctuation = token(l.OPERATOR,
  P'=>' + P'.>' + P'<:' + P'->'
  + S('=.,:;()[]{}!?~^&|_@'))

-- highlight functions with syntax sugar at declaration
local func
  = token(l.KEYWORD, pword{'fun', 'new', 'be'}) * ws^-1
  * annotation^-1 * ws^-1
  * capability^-1 * ws^-1
  * token(l.FUNCTION, pword{
    'create', 'dispose', '_final', 'apply', 'update',
    'add', 'sub', 'mul', 'div', 'mod', 'add_unsafe', 'sub_unsafe',
    'mul_unsafe', 'div_unsafe', 'mod_unsafe', 'shl', 'shr', 'shl_unsafe',
    'shr_unsafe', 'op_and', 'op_or', 'op_xor', 'eq', 'ne', 'lt', 'le', 'ge',
    'gt', 'eq_unsafe', 'ne_unsafe', 'lt_unsafe', 'le_unsafe', 'ge_unsafe',
    'gt_unsafe', 'neg', 'neg_unsafe', 'op_not',
    'has_next', 'next',
    '_serialise_space', '_serialise', '_deserialise'})

local id_suffix = (l.alnum + P("'") + P('_'))^0
local type = token(l.TYPE, P('_')^-1 * l.upper * id_suffix)
local identifier = token(l.IDENTIFIER, P('_')^-1 * l.lower * id_suffix)
local tuple_lookup = token(l.IDENTIFIER, P('_') * l.digit^1)

local M = {_NAME = 'pony'}

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'annotation', annotation},
  {'boolean', lit_bool},
  {'number', lit_num},
  {'string', lit_str},
  {'function', func},
  {'keyword', keyword},
  {'capability', capability},
  {'qualifier', qualifier},
  {'operator', operator},
  {'type', type},
  {'identifier', identifier},
  {'lookup', tuple_lookup},
  {'punctuation', punctuation}
}

return M
