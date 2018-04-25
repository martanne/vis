-- Fantom LPeg lexer.
-- Based on Java LPeg lexer by Mitchell mitchell.att.foicica.com and Vim's Fantom syntax.
-- By MarSoft.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'fantom'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^2)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local doc_comment = '**' * l.nonnewline_esc^0
local comment = token(l.COMMENT, line_comment + block_comment + doc_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('LlFfDd')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'using', 'native', -- external
  'goto', 'void', 'serializable', 'volatile', -- error
  'if', 'else', 'switch', -- conditional
  'do', 'while', 'for', 'foreach', 'each', -- repeat
  'true', 'false', -- boolean
  'null', -- constant
  'this', 'super', -- typedef
  'new', 'is', 'isnot', 'as', -- operator
  'plus', 'minus', 'mult', 'div', 'mod', 'get', 'set', 'slice', 'lshift', 'rshift', 'and', 'or', 'xor', 'inverse', 'negate', 'increment', 'decrement', 'equals', 'compare', -- long operator
  'return', -- stmt
  'static', 'const', 'final', -- storage class
  'virtual', 'override', 'once', -- slot
  'readonly', -- field
  'throw', 'try', 'catch', 'finally', -- exceptions
  'assert', -- assert
  'class', 'enum', 'mixin', -- typedef
  'break', 'continue', -- branch
  'default', 'case',  -- labels
  'public', 'internal', 'protected', 'private', 'abstract', -- scope decl
})

-- Types.
local type = token(l.TYPE, word_match{
  'Void', 'Bool', 'Int', 'Float', 'Decimal',
  'Str', 'Duration', 'Uri', 'Type', 'Range',
  'List', 'Map', 'Obj',
  'Err', 'Env',
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}#'))

-- Annotations.
local facet = token('facet', '@' * l.word)

-- Functions.
local func = token(l.FUNCTION, l.word) * #P('(')

-- Classes.
local class_sequence = token(l.KEYWORD, P('class')) * ws^1 *
                       token(l.TYPE, l.word) * ( -- at most one inheritance spec
                         ws^1 * token(l.OPERATOR, P(':')) * ws^1 *
                         token(l.TYPE, l.word) *
                         ( -- at least 0 (i.e. any number) of additional classes
                           ws^0 * token(l.OPERATOR, P(',')) * ws^0 * token(l.TYPE, l.word)
                         )^0
                       )^-1

M._rules = {
  {'whitespace', ws},
  {'class', class_sequence},
  {'keyword', keyword},
  {'type', type},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'facet', facet},
  {'operator', operator},
}

M._tokenstyles = {
  facet = l.STYLE_PREPROCESSOR
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
