-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Lisp LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Identifiers.
local word = lexer.alpha * (lexer.alnum + S('_-'))^0
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, word))

-- Strings.
local character = lexer.word_match{
	'backspace', 'linefeed', 'newline', 'page', 'return', 'rubout', 'space', 'tab'
} + 'x' * lexer.xdigit^1 + lexer.any
lex:add_rule('string', lex:tag(lexer.STRING, "'" * word + lexer.range('"') + '#\\' * character))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = lexer.range('#|', '|#')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number',
	lex:tag(lexer.NUMBER, P('-')^-1 * lexer.digit^1 * (S('./') * lexer.digit^1)^-1))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('<>=*/+-`@%()')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '#|', '|#')

-- Word lists
lex:set_word_list(lexer.KEYWORD, {
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
})

lexer.property['scintillua.comment'] = ';'

return lex
