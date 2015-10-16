-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- C++ LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'cpp'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = P('L')^-1 * l.delimited_range("'", true)
local dq_str = P('L')^-1 * l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Preprocessor.
local preproc_word = word_match{
  'define', 'elif', 'else', 'endif', 'error', 'if', 'ifdef', 'ifndef', 'import',
  'include', 'line', 'pragma', 'undef', 'using', 'warning'
}
local preproc = token(l.PREPROCESSOR,
                      l.starts_line('#') * S('\t ')^0 * preproc_word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'asm', 'auto', 'break', 'case', 'catch', 'class', 'const', 'const_cast',
  'continue', 'default', 'delete', 'do', 'dynamic_cast', 'else', 'explicit',
  'export', 'extern', 'false', 'for', 'friend', 'goto', 'if', 'inline',
  'mutable', 'namespace', 'new', 'operator', 'private', 'protected', 'public',
  'register', 'reinterpret_cast', 'return', 'sizeof', 'static', 'static_cast',
  'switch', 'template', 'this', 'throw', 'true', 'try', 'typedef', 'typeid',
  'typename', 'using', 'virtual', 'volatile', 'while',
  -- Operators
  'and', 'and_eq', 'bitand', 'bitor', 'compl', 'not', 'not_eq', 'or', 'or_eq',
  'xor', 'xor_eq',
  -- C++11
  'alignas', 'alignof', 'constexpr', 'decltype', 'final', 'noexcept',
  'override', 'static_assert', 'thread_local'
})

-- Types.
local type = token(l.TYPE, word_match{
  'bool', 'char', 'double', 'enum', 'float', 'int', 'long', 'short', 'signed',
  'struct', 'union', 'unsigned', 'void', 'wchar_t',
  -- C++11
  'char16_t', 'char32_t', 'nullptr'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'%l+', '[{}]', '/%*', '%*/', '//'},
  [l.PREPROCESSOR] = {
    region = 1, endregion = -1,
    ['if'] = 1, ifdef = 1, ifndef = 1, endif = -1
  },
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
