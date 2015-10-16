-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Lisp LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'lisp'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = ';' * l.nonnewline^0
local block_comment = '#|' * (l.any - '|#')^0 * P('|#')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

local word = l.alpha * (l.alnum + '_' + '-')^0

-- Strings.
local literal = "'" * word
local dq_str = l.delimited_range('"')
local string = token(l.STRING, literal + dq_str)

-- Numbers.
local number = token(l.NUMBER, P('-')^-1 * l.digit^1 * (S('./') * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'defclass', 'defconstant', 'defgeneric', 'define-compiler-macro',
  'define-condition', 'define-method-combination', 'define-modify-macro',
  'define-setf-expander', 'define-symbol-macro', 'defmacro', 'defmethod',
  'defpackage', 'defparameter', 'defsetf', 'defstruct', 'deftype', 'defun',
  'defvar',
  'abort', 'assert', 'block', 'break', 'case', 'catch', 'ccase', 'cerror',
  'cond', 'ctypecase', 'declaim', 'declare', 'do', 'do*', 'do-all-symbols',
  'do-external-symbols', 'do-symbols', 'dolist', 'dotimes', 'ecase', 'error',
  'etypecase', 'eval-when', 'flet', 'handler-bind', 'handler-case', 'if',
  'ignore-errors', 'in-package', 'labels', 'lambda', 'let', 'let*', 'locally',
  'loop', 'macrolet', 'multiple-value-bind', 'proclaim', 'prog', 'prog*',
  'prog1', 'prog2', 'progn', 'progv', 'provide', 'require', 'restart-bind',
  'restart-case', 'restart-name', 'return', 'return-from', 'signal',
  'symbol-macrolet', 'tagbody', 'the', 'throw', 'typecase', 'unless',
  'unwind-protect', 'when', 'with-accessors', 'with-compilation-unit',
  'with-condition-restarts', 'with-hash-table-iterator',
  'with-input-from-string', 'with-open-file', 'with-open-stream',
  'with-output-to-string', 'with-package-iterator', 'with-simple-restart',
  'with-slots', 'with-standard-io-syntax',
  't', 'nil'
}, '-'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('<>=*/+-`@%()'))

-- Entities.
local entity = token('entity', '&' * word)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
  {'entity', entity},
}

M._tokenstyles = {
  entity = l.STYLE_VARIABLE
}

M._foldsymbols = {
  _patterns = {'[%(%)%[%]{}]', '#|', '|#', ';'},
  [l.OPERATOR] = {
    ['('] = 1, [')'] = -1, ['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1
  },
  [l.COMMENT] = {['#|'] = 1, ['|#'] = -1, [';'] = l.fold_line_comments(';')}
}

return M
