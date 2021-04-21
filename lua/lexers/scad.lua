-- cribbed almost entirely from cpp.lua

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'scad'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  -- reuse
  'function', 'module',

  -- conditional
  'if', 'else',

  -- repetition
  'for', 'intersection_for',

  -- list comprehension
  'each', 'let',

  -- inclusion
  'include', 'use',

  -- boolean
  'difference', 'intersection', 'union',

  -- transformation
  'color', 'hull', 'linear_extrude', 'minkowski', 'mirror', 'multmatrix',
  'offset', 'projection', 'rotate', 'rotate_extrude', 'scale', 'translate',

  -- primitive solids
  'cube', 'cylinder', 'polyhedron', 'sphere', 'surface',

  -- primitive 2D
  'circle', 'import_dxf', 'polygon', 'square', 'text',

  -- primitive import
  'child', 'children', 'import',

  -- other
  'assert', 'echo', 'render',
})

local values = token(l.KEYWORD, word_match{
  'false', 'true', 'undef',
})

local builtins = token(l.KEYWORD, word_match{
  -- arithmetic
  'abs', 'acos', 'asin', 'atan', 'atan2', 'ceil', 'cos', 'cross', 'exp',
  'floor', 'ln', 'log', 'max', 'min', 'norm', 'pow', 'rands', 'round', 'sign',
  'sin', 'sqrt', 'tan',

  -- strings
  'chr', 'ord', 'str',

  -- vectors/lists
  'concat', 'len', 'lookup', 'search',

  -- dxf
  'dxf_cross', 'dxf_dim',

  -- other
  'parent_module', 'version', 'version_num',

  -- type checks
  'is_undef', 'is_bool', 'is_num', 'is_string', 'is_list', 'is_function',
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, P('$')^-1 * l.word)

-- Operators.
local operator = token(l.OPERATOR, S('#+-/*%<>!=^&|?~:;,.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'value', values},
  {'builtin', builtins},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'%l+', '[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')},
}

return M
