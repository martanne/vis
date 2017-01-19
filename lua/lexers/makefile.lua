-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Makefile LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'makefile'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Keywords.
local keyword = token(l.KEYWORD, P('!')^-1 * l.word_match({
  -- GNU Make conditionals.
  'ifeq', 'ifneq', 'ifdef', 'ifndef', 'else', 'endif',
  -- Other conditionals.
  'if', 'elseif', 'elseifdef', 'elseifndef',
  -- Directives and other keywords.
  'define', 'endef', 'export', 'include', 'override', 'private', 'undefine',
  'unexport', 'vpath'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, l.word_match({
  -- Functions for String Substitution and Analysis.
  'subst', 'patsubst', 'strip', 'findstring', 'filter', 'filter-out', 'sort',
  'word', 'wordlist', 'words', 'firstword', 'lastword',
  -- Functions for File Names.
  'dir', 'notdir', 'suffix', 'basename', 'addsuffix', 'addprefix', 'join',
  'wildcard', 'realpath', 'abspath',
  -- Functions for Conditionals.
  'if', 'or', 'and',
  -- Miscellaneous Functions.
  'foreach', 'call', 'value', 'eval', 'origin', 'flavor', 'shell',
  -- Functions That Control Make.
  'error', 'warning', 'info'
}), '-')

-- Variables.
local word_char, assign = l.any - l.space - S(':#=(){}'), S(':+?')^-1 * '='
local expanded_var = '$' * ('(' * word_char^1 * ')' + '{' * word_char^1 * '}')
local auto_var = '$' * S('@%<?^+|*')
local special_var = l.word_match({
  'MAKEFILE_LIST', '.DEFAULT_GOAL', 'MAKE_RESTARTS', '.RECIPEPREFIX',
  '.VARIABLES', '.FEATURES', '.INCLUDE_DIRS',
  'GPATH', 'MAKECMDGOALS', 'MAKESHELL', 'SHELL', 'VPATH'
}, '.') * #(ws^0 * assign)
local implicit_var = l.word_match{
  -- Some common variables.
  'AR', 'AS', 'CC', 'CXX', 'CPP', 'FC', 'M2C', 'PC', 'CO', 'GET', 'LEX', 'YACC',
  'LINT', 'MAKEINFO', 'TEX', 'TEXI2DVI', 'WEAVE', 'CWEAVE', 'TANGLE', 'CTANGLE',
  'RM',
  -- Some common flag variables.
  'ARFLAGS', 'ASFLAGS', 'CFLAGS', 'CXXFLAGS', 'COFLAGS', 'CPPFLAGS', 'FFLAGS',
  'GFLAGS', 'LDFLAGS', 'LFLAGS', 'YFLAGS', 'PFLAGS', 'RFLAGS', 'LINTFLAGS',
  -- Other.
  'DESTDIR', 'MAKE', 'MAKEFLAGS', 'MAKEOVERRIDES', 'MFLAGS'
} * #(ws^0 * assign)
local computed_var = token(l.OPERATOR, '$' * S('({')) * func
local variable = token(l.VARIABLE,
                       expanded_var + auto_var + special_var + implicit_var) +
                 computed_var

-- Targets.
local special_target = token(l.CONSTANT, l.word_match({
  '.PHONY', '.SUFFIXES', '.DEFAULT', '.PRECIOUS', '.INTERMEDIATE', '.SECONDARY',
  '.SECONDEXPANSION', '.DELETE_ON_ERROR', '.IGNORE', '.LOW_RESOLUTION_TIME',
  '.SILENT', '.EXPORT_ALL_VARIABLES', '.NOTPARALLEL', '.ONESHELL', '.POSIX'
}, '.'))
local normal_target = token('target', (l.any - l.space - S(':#='))^1)
local target = l.starts_line((special_target + normal_target) * ws^0 *
                             #(':' * -P('=')))

-- Identifiers.
local identifier = token(l.IDENTIFIER, word_char^1)

-- Operators.
local operator = token(l.OPERATOR, assign + S(':$(){}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'target', target},
  {'variable', variable},
  {'operator', operator},
  {'identifier', identifier},
  {'comment', comment},
}

M._tokenstyles = {
  target = l.STYLE_LABEL
}

M._LEXBYLINE = true

-- Embedded Bash.
local bash = l.load('bash')
bash._RULES['variable'] = token(l.VARIABLE, '$$' * word_char^1) +
                          bash._RULES['variable'] + variable
local bash_start_rule = token(l.WHITESPACE, P('\t')) + token(l.OPERATOR, P(';'))
local bash_end_rule = token(l.WHITESPACE, P('\n'))
l.embed_lexer(M, bash, bash_start_rule, bash_end_rule)

return M
