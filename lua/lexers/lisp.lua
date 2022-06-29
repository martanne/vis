-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Lisp LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('lisp')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'defclass', 'defconstant', 'defgeneric', 'define-compiler-macro', 'define-condition',
  'define-method-combination', 'define-modify-macro', 'define-setf-expander', 'define-symbol-macro',
  'defmacro', 'defmethod', 'defpackage', 'defparameter', 'defsetf', 'defstruct', 'deftype', 'defun',
  'defvar', --
  'abort', 'assert', 'block', 'break', 'case', 'catch', 'ccase', 'cerror', 'cond', 'ctypecase',
  'declaim', 'declare', 'do', 'do*', 'do-all-symbols', 'do-external-symbols', 'do-symbols',
  'dolist', 'dotimes', 'ecase', 'error', 'etypecase', 'eval-when', 'flet', 'handler-bind',
  'handler-case', 'if', 'ignore-errors', 'in-package', 'labels', 'lambda', 'let', 'let*', 'locally',
  'loop', 'macrolet', 'multiple-value-bind', 'proclaim', 'prog', 'prog*', 'prog1', 'prog2', 'progn',
  'progv', 'provide', 'require', 'restart-bind', 'restart-case', 'restart-name', 'return',
  'return-from', 'signal', 'symbol-macrolet', 'tagbody', 'the', 'throw', 'typecase', 'unless',
  'unwind-protect', 'when', 'with-accessors', 'with-compilation-unit', 'with-condition-restarts',
  'with-hash-table-iterator', 'with-input-from-string', 'with-open-file', 'with-open-stream',
  'with-output-to-string', 'with-package-iterator', 'with-simple-restart', 'with-slots',
  'with-standard-io-syntax', --
  't', 'nil'
}))

-- Identifiers.
local word = lexer.alpha * (lexer.alnum + '_' + '-')^0
lex:add_rule('identifier', token(lexer.IDENTIFIER, word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, "'" * word + lexer.range('"') + '#\\' * lexer.any))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = lexer.range('#|', '|#')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, P('-')^-1 * lexer.digit^1 * (S('./') * lexer.digit^1)^-1))

-- Entities.
lex:add_rule('entity', token('entity', '&' * word))
lex:add_style('entity', lexer.styles.variable)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('<>=*/+-`@%()')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '#|', '|#')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines(';'))

return lex
