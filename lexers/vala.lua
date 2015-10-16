-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Vala LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'vala'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local tq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local ml_str = '@' * l.delimited_range('"', false, true)
local string = token(l.STRING, tq_str + sq_str + dq_str + ml_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('uUlLfFdDmM')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'class', 'delegate', 'enum', 'errordomain', 'interface', 'namespace',
  'signal', 'struct', 'using',
  -- Modifiers.
  'abstract', 'const', 'dynamic', 'extern', 'inline', 'out', 'override',
  'private', 'protected', 'public', 'ref', 'static', 'virtual', 'volatile',
  'weak',
  -- Other.
  'as', 'base', 'break', 'case', 'catch', 'construct', 'continue', 'default',
  'delete', 'do', 'else', 'ensures', 'finally', 'for', 'foreach', 'get', 'if',
  'in', 'is', 'lock', 'new', 'requires', 'return', 'set', 'sizeof', 'switch',
  'this', 'throw', 'throws', 'try', 'typeof', 'value', 'var', 'void', 'while',
  -- Etc.
  'null', 'true', 'false'
})

-- Types.
local type = token(l.TYPE, word_match{
  'bool', 'char', 'double', 'float', 'int', 'int8', 'int16', 'int32', 'int64',
  'long', 'short', 'size_t', 'ssize_t', 'string', 'uchar', 'uint', 'uint8',
  'uint16', 'uint32', 'uint64', 'ulong', 'unichar', 'ushort'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
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
