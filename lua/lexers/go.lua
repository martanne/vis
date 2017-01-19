-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Go LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'go'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * '*/'
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local raw_str = l.delimited_range('`', false, true)
local string = token(l.STRING, sq_str + dq_str + raw_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * P('i')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'break', 'case', 'chan', 'const', 'continue', 'default', 'defer', 'else',
  'fallthrough', 'for', 'func', 'go', 'goto', 'if', 'import', 'interface',
  'map', 'package', 'range', 'return', 'select', 'struct', 'switch', 'type',
  'var'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'true', 'false', 'iota', 'nil'
})

-- Types.
local type = token(l.TYPE, word_match{
  'bool', 'byte', 'complex64', 'complex128', 'error', 'float32', 'float64',
  'int', 'int8', 'int16', 'int32', 'int64', 'rune', 'string', 'uint', 'uint8',
  'uint16', 'uint32', 'uint64', 'uintptr'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'append', 'cap', 'close', 'complex', 'copy', 'delete', 'imag', 'len', 'make',
  'new', 'panic', 'print', 'println', 'real', 'recover'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-*/%&|^<>=!:;.,()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'constant', constant},
  {'type', type},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
