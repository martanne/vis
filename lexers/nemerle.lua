-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Nemerle LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'nemerle'}

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
  'define', 'elif', 'else', 'endif', 'endregion', 'error', 'if', 'ifdef',
  'ifndef', 'line', 'pragma', 'region', 'undef', 'using', 'warning'
}
local preproc = token(l.PREPROCESSOR,
                      l.starts_line('#') * S('\t ')^0 * preproc_word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  '_', 'abstract', 'and', 'array', 'as', 'base', 'catch', 'class', 'def', 'do',
  'else', 'extends', 'extern', 'finally', 'foreach', 'for',  'fun', 'if',
  'implements', 'in', 'interface', 'internal', 'lock', 'macro', 'match',
  'module', 'mutable', 'namespace', 'new', 'out', 'override', 'params',
  'private', 'protected', 'public', 'ref', 'repeat', 'sealed', 'static',
  'struct', 'syntax', 'this', 'throw', 'try', 'type', 'typeof', 'unless',
  'until', 'using', 'variant', 'virtual', 'when', 'where', 'while',
  -- Values.
  'null', 'true', 'false'
})

-- Types.
local type = token(l.TYPE, word_match{
  'bool', 'byte', 'char', 'decimal', 'double', 'float', 'int', 'list', 'long',
  'object', 'sbyte', 'short', 'string', 'uint', 'ulong', 'ushort', 'void'
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
