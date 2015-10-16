-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Objective C LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'objective_c'}

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
  'define', 'elif', 'else', 'endif', 'error', 'if', 'ifdef',
  'ifndef', 'import', 'include', 'line', 'pragma', 'undef',
  'warning'
}
local preproc = token(l.PREPROCESSOR,
                      l.starts_line('#') * S('\t ')^0 * preproc_word *
                      (l.nonnewline_esc^1 + l.space * l.nonnewline_esc^0))

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  -- From C.
  'asm', 'auto', 'break', 'case', 'const', 'continue', 'default', 'do', 'else',
  'extern', 'false', 'for', 'goto', 'if', 'inline', 'register', 'return',
  'sizeof', 'static', 'switch', 'true', 'typedef', 'void', 'volatile', 'while',
  'restrict', '_Bool', '_Complex', '_Pragma', '_Imaginary',
  -- Objective C.
  'oneway', 'in', 'out', 'inout', 'bycopy', 'byref', 'self', 'super',
  -- Preprocessor directives.
  '@interface', '@implementation', '@protocol', '@end', '@private',
  '@protected', '@public', '@class', '@selector', '@encode', '@defs',
  '@synchronized', '@try', '@throw', '@catch', '@finally',
  -- Constants.
  'TRUE', 'FALSE', 'YES', 'NO', 'NULL', 'nil', 'Nil', 'METHOD_NULL'
}, '@'))

-- Types.
local type = token(l.TYPE, word_match{
  'apply_t', 'id', 'Class', 'MetaClass', 'Object', 'Protocol', 'retval_t',
  'SEL', 'STR', 'IMP', 'BOOL', 'TypedStream'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'string', string},
  {'identifier', identifier},
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
