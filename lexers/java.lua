-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Java LPeg lexer.
-- Modified by Brian Schott.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'java'}

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

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('LlFfDd')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'abstract', 'assert', 'break', 'case', 'catch', 'class', 'const', 'continue',
  'default', 'do', 'else', 'enum', 'extends', 'final', 'finally', 'for', 'goto',
  'if', 'implements', 'import', 'instanceof', 'interface', 'native', 'new',
  'package', 'private', 'protected', 'public', 'return', 'static', 'strictfp',
  'super', 'switch', 'synchronized', 'this', 'throw', 'throws', 'transient',
  'try', 'while', 'volatile',
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
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}'))

-- Annotations.
local annotation = token('annotation', '@' * l.word)

-- Functions.
local func = token(l.FUNCTION, l.word) * #P('(')

-- Classes.
local class_sequence = token(l.KEYWORD, P('class')) * ws^1 *
                       token(l.CLASS, l.word)

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
  {'annotation', annotation},
  {'operator', operator},
}

M._tokenstyles = {
  annotation = l.STYLE_PREPROCESSOR
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
