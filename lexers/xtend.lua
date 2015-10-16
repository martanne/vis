-- Copyright (c) 2014-2015 Piotr Orzechowski [drzewo.org]. See LICENSE.
-- Xtend LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'xtend'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Templates.
local templ_str = "'''" * (l.any - P("'''"))^0 * P("'''")^-1
local template = token('template', templ_str, true)

-- Numbers.
local small_suff = S('lL')
local med_suff = P(S('bB') * S('iI'))
local large_suff = S('dD') + S('fF') + P(S('bB') * S('dD'))
local exp = S('eE') * l.digit^1

local dec_inf = ('_' * l.digit^1)^0
local hex_inf = ('_' * l.xdigit^1)^0
local float_pref = l.digit^1 * '.' * l.digit^1
local float_suff = exp^-1 * med_suff^-1 * large_suff^-1

local dec = l.digit * dec_inf * (small_suff^-1 + float_suff)
local hex = l.hex_num * hex_inf * P('#' * (small_suff + med_suff))^-1
local float = float_pref * dec_inf * float_suff

local number = token(l.NUMBER, float + hex + dec)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  -- General.
  'abstract', 'annotation', 'as', 'case', 'catch', 'class', 'create', 'def',
  'default', 'dispatch', 'do', 'else', 'enum', 'extends', 'extension', 'final',
  'finally', 'for', 'if', 'implements', 'import', 'interface', 'instanceof',
  'it', 'new', 'override', 'package', 'private', 'protected', 'public',
  'return', 'self', 'static', 'super', 'switch', 'synchronized', 'this',
  'throw', 'throws', 'try', 'typeof', 'val', 'var', 'while',
  -- Templates.
  -- 'AFTER', 'BEFORE', 'ENDFOR', 'ENDIF', 'FOR', 'IF', 'SEPARATOR',
  -- Literals.
  'true', 'false', 'null'
})

-- Types.
local type = token(l.TYPE, word_match{
  'boolean', 'byte', 'char', 'double', 'float', 'int', 'long', 'short', 'void',
  'Boolean', 'Byte', 'Character', 'Double', 'Float', 'Integer', 'Long', 'Short',
  'String'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}#'))

-- Annotations.
local annotation = token('annotation', '@' * l.word)

-- Functions.
local func = token(l.FUNCTION, l.word) * #P('(')

-- Classes.
local class = token(l.KEYWORD, P('class')) * ws^1 * token(l.CLASS, l.word)

-- Rules.
M._rules = {
  {'whitespace', ws},
  {'class', class},
  {'keyword', keyword},
  {'type', type},
  {'function', func},
  {'identifier', identifier},
  {'template', template},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'annotation', annotation},
  {'operator', operator},
  {'error', token(l.ERROR, l.any)},
}

-- Token styles.
M._tokenstyles = {
  annotation = l.STYLE_PREPROCESSOR,
  template = l.STYLE_EMBEDDED
}

-- Folding.
M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//', 'import'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')},
  [l.KEYWORD] = {['import'] = l.fold_line_comments('import')}
}

return M
