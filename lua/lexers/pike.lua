-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Pike LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'pike'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local nested_comment = l.nested_pair('/*', '*/')
local comment = token(l.COMMENT, line_comment + nested_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local lit_str = '#' * l.delimited_range('"')
local string = token(l.STRING, sq_str + dq_str + lit_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('lLdDfF')^-1)

-- Preprocessors.
local preproc = token(l.PREPROCESSOR, l.starts_line('#') * l.nonnewline^0)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'break', 'case', 'catch', 'continue', 'default', 'do', 'else', 'for',
  'foreach', 'gauge', 'if', 'lambda', 'return', 'sscanf', 'switch', 'while',
  'import', 'inherit',
  -- Type modifiers.
  'constant', 'extern', 'final', 'inline', 'local', 'nomask', 'optional',
  'private', 'protected', 'public', 'static', 'variant'
})

-- Types.
local type = token(l.TYPE, word_match{
  'array', 'class', 'float', 'function', 'int', 'mapping', 'mixed', 'multiset',
  'object', 'program', 'string', 'void'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('<>=!+-/*%&|^~@`.,:;()[]{}'))

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
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
