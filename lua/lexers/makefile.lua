-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Makefile LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('makefile', {lex_by_line = true})

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, P('!')^-1 * word_match({
  -- GNU Make conditionals.
  'ifeq', 'ifneq', 'ifdef', 'ifndef', 'else', 'endif',
  -- Other conditionals.
  'if', 'elseif', 'elseifdef', 'elseifndef',
  -- Directives and other keywords.
  'define', 'endef', 'export', 'include', 'override', 'private', 'undefine', 'unexport', 'vpath'
}, true)))

-- Targets.
local special_target = token(lexer.CONSTANT, word_match{
  '.PHONY', '.SUFFIXES', '.DEFAULT', '.PRECIOUS', '.INTERMEDIATE', '.SECONDARY', '.SECONDEXPANSION',
  '.DELETE_ON_ERROR', '.IGNORE', '.LOW_RESOLUTION_TIME', '.SILENT', '.EXPORT_ALL_VARIABLES',
  '.NOTPARALLEL', '.ONESHELL', '.POSIX'
})
local normal_target = token('target', (lexer.any - lexer.space - S(':#='))^1)
local target_list = normal_target * (ws * normal_target)^0
lex:add_rule('target', lexer.starts_line((special_target + target_list) * ws^0 * #(':' * -P('='))))
lex:add_style('target', lexer.styles.label)

-- Variables.
local word_char = lexer.any - lexer.space - S(':#=(){}')
local assign = S(':+?')^-1 * '='
local expanded_var = '$' * ('(' * word_char^1 * ')' + '{' * word_char^1 * '}')
local auto_var = '$' * S('@%<?^+|*')
local special_var = word_match{
  'MAKEFILE_LIST', '.DEFAULT_GOAL', 'MAKE_RESTARTS', '.RECIPEPREFIX', '.VARIABLES', '.FEATURES',
  '.INCLUDE_DIRS', 'GPATH', 'MAKECMDGOALS', 'MAKESHELL', 'SHELL', 'VPATH'
} * #(ws^0 * assign)
local implicit_var = word_match{
  -- Some common variables.
  'AR', 'AS', 'CC', 'CXX', 'CPP', 'FC', 'M2C', 'PC', 'CO', 'GET', 'LEX', 'YACC', 'LINT', 'MAKEINFO',
  'TEX', 'TEXI2DVI', 'WEAVE', 'CWEAVE', 'TANGLE', 'CTANGLE', 'RM',
  -- Some common flag variables.
  'ARFLAGS', 'ASFLAGS', 'CFLAGS', 'CXXFLAGS', 'COFLAGS', 'CPPFLAGS', 'FFLAGS', 'GFLAGS', 'LDFLAGS',
  'LDLIBS', 'LFLAGS', 'YFLAGS', 'PFLAGS', 'RFLAGS', 'LINTFLAGS',
  -- Other.
  'DESTDIR', 'MAKE', 'MAKEFLAGS', 'MAKEOVERRIDES', 'MFLAGS'
} * #(ws^0 * assign)
local variable = token(lexer.VARIABLE, expanded_var + auto_var + special_var + implicit_var)
local computed_var = token(lexer.OPERATOR, '$' * S('({')) * token(lexer.FUNCTION, word_match{
  -- Functions for String Substitution and Analysis.
  'subst', 'patsubst', 'strip', 'findstring', 'filter', 'filter-out', 'sort', 'word', 'wordlist',
  'words', 'firstword', 'lastword',
  -- Functions for File Names.
  'dir', 'notdir', 'suffix', 'basename', 'addsuffix', 'addprefix', 'join', 'wildcard', 'realpath',
  'abspath',
  -- Functions for Conditionals.
  'if', 'or', 'and',
  -- Miscellaneous Functions.
  'foreach', 'call', 'value', 'eval', 'origin', 'flavor', 'shell',
  -- Functions That Control Make.
  'error', 'warning', 'info'
})
lex:add_rule('variable', variable + computed_var)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, assign + S(':$(){}')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, word_char^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Embedded Bash.
local bash = lexer.load('bash')
bash:modify_rule('variable',
  token(lexer.VARIABLE, '$$' * word_char^1) + bash:get_rule('variable') + variable)
local bash_start_rule = token(lexer.WHITESPACE, '\t') + token(lexer.OPERATOR, ';')
local bash_end_rule = token(lexer.WHITESPACE, '\n')
lex:embed(bash, bash_start_rule, bash_end_rule)

return lex
