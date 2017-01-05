-- Copyright 2006-2016 Mitchell mitchell.att.foicica.com. See LICENSE.
-- C LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, B = lpeg.P, lpeg.R, lpeg.S, lpeg.B

local M = {_NAME = 'ansi_c'}

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
  'define', 'elif', 'else', 'endif', 'if', 'ifdef', 'ifndef', 'include', 'line',
  'pragma', 'undef'
}
local preproc = token(l.PREPROCESSOR,
                      l.starts_line('#') * S('\t ')^0 * preproc_word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'auto', 'break', 'case', 'const', 'continue', 'default', 'do', 'else',
  'extern', 'for', 'goto', 'if', 'inline', 'register', 'restrict', 'return',
  'sizeof', 'static', 'switch', 'typedef', 'volatile', 'while'
})

-- Types.
 local type = token(l.TYPE, word_match{
  'char', 'double', 'enum', 'float', 'int', 'long', 'short',
  'signed', 'struct', 'union', 'unsigned', 'void', '_Bool', '_Complex',
  '_Imaginary', 'bool', 'ptrdiff_t', 'size_t', 'max_align_t', 'wchar_t',
  'intptr_t', 'uintptr_t', 'intmax_t', 'uintmax_t'} +
  (P('u')^-1 * P('int') * (P('_least') + P('_fast'))^-1 * l.dec_num^1 * P('_t'))
)

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}'))

-- Labels.
local label = token(l.LABEL, (-(B(P('case ')))) * (l.alnum + P('_'))^1 * P(':'))

M._rules = {
  {'whitespace', ws},
  {'label', label},
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
  [l.PREPROCESSOR] = {['if'] = 1, ifdef = 1, ifndef = 1, endif = -1},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
