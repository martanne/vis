-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Makefile LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(..., {lex_by_line = true})

-- Function definition.
local word = (lexer.any - lexer.space - S('$:,#=(){}'))^1
local func_name = lex:tag(lexer.FUNCTION, word)
local ws = lex:get_rule('whitespace')
local eq = lex:tag(lexer.OPERATOR, '=')
lex:add_rule('function_def', lex:tag(lexer.KEYWORD, lexer.word_match('define')) * ws * func_name *
	ws^-1 * (eq + -1))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, P('!')^-1 * lex:word_match(lexer.KEYWORD, true)))

-- Targets.
local special_target = lex:tag(lexer.CONSTANT_BUILTIN, '.' * lex:word_match('special_targets'))
-- local normal_target = lex:tag('target', (lexer.any - lexer.space - S(':+?!=#'))^1)
local target = special_target -- + normal_target * (ws * normal_target)^0
lex:add_rule('target', lexer.starts_line(target * ws^-1 * #(':' * lexer.space)))

-- Variable and function assignments.
local func_assign = func_name * ws^-1 * eq *
	#P(function(input, index) return input:find('%$%(%d%)', index) end)
local builtin_var = lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN))
local var_name = lex:tag(lexer.VARIABLE, word)
local var_assign = (builtin_var + var_name) * ws^-1 *
	lex:tag(lexer.OPERATOR, S(':+?!')^-1 * '=' + '::=')
lex:add_rule('assign', lexer.starts_line(func_assign + var_assign, true) + B(': ') * var_assign)

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S(':(){}|')))

-- Strings.
lex:add_rule('string', lexer.range("'", true) + lexer.range('"', true))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, word))

-- Functions.
local builtin_func = lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local call_func = lex:tag(lexer.FUNCTION_BUILTIN, 'call') * ws * func_name
local func = lex:tag(lexer.OPERATOR, '$' * S('({')) * (call_func + builtin_func)
lex:add_rule('function', func)

-- Variables.
local auto_var = lex:tag(lexer.OPERATOR, '$') * lex:tag(lexer.VARIABLE_BUILTIN, S('@%<?^+|*')) +
	lex:tag(lexer.OPERATOR, '$(') * lex:tag(lexer.VARIABLE_BUILTIN, S('@%<?^+*') * S('DF'))
local var_ref = lex:tag(lexer.OPERATOR, P('$(') + '${') * (builtin_var + var_name)
local var = auto_var + var_ref
lex:add_rule('variable', var)

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Embedded Bash in target rules.
local bash = lexer.load('bash')
bash:modify_rule('variable',
	lex:tag(lexer.VARIABLE, '$$' * word) + func + var + bash:get_rule('variable'))
local bash_start_rule = lex:tag(lexer.WHITESPACE, '\t') + lex:tag(lexer.OPERATOR, ';')
local bash_end_rule = lex:tag(lexer.WHITESPACE, '\n')
lex:embed(bash, bash_start_rule, bash_end_rule)
-- Embedded Bash in $(shell ...) calls.
local shell = lexer.load('bash', 'bash.shell')
bash_start_rule = #P('$(shell') * func
bash_end_rule = -B('\\') * lex:tag(lexer.OPERATOR, ')')
lex:embed(shell, bash_start_rule, bash_end_rule)

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'define', 'endef', -- multi-line
	'else', 'endif', 'ifdef', 'ifeq', 'ifndef', 'ifneq', -- conditionals
	'export', 'include', 'load', 'override', 'undefine', 'unexport', 'vpath', -- directives
	'private', --
	'if', 'elseif', 'elseifdef', 'elseifndef' -- non-Make conditionals
})

lex:set_word_list('special_targets', {
	'DEFAULT', 'DELETE_ON_ERROR', 'EXPORT_ALL_VARIABLES', 'IGNORE', 'INTERMEDIATE',
	'LOW_RESOLUTION_TIME', 'NOTPARALLEL', 'ONESHELL', 'PHONY', 'POSIX', 'PRECIOUS', 'SECONDARY',
	'SECONDEXPANSION', 'SILENT', 'SUFFIXES'
})

lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	-- Special.
	'.DEFAULT_GOAL', '.FEATURES', '.INCLUDE_DIRS', '.LIBPATTERNS', '.LOADED', '.RECIPEPREFIX',
	'.SHELLFLAGS', '.SHELLSTATUS', '.VARIABLES', --
	'COMSPEC', 'MAKESHELL', 'SHELL', -- choosing the shell
	'GPATH', 'VPATH', -- search
	-- Make.
	'MAKE', 'MAKECMDGOALS', 'MAKEFILES', 'MAKEFILE_LIST', 'MAKEFLAGS', 'MAKELEVEL', 'MAKEOVERRIDES',
	'MAKE_RESTARTS', 'MAKE_TERMERR', 'MAKE_TERMOUT', 'MFLAGS',
	-- Other.
	'CURDIR', 'OUTPUT_OPTION', 'SUFFIXES',
	-- Implicit.
	'AR', 'ARFLAGS', 'AS', 'ASFLAGS', 'CC', 'CFLAGS', 'CO', 'COFLAGS', 'CPP', 'CPPFLAGS', 'CTANGLE',
	'CWEAVE', 'CXX', 'CXXFLAGS', 'FC', 'FFLAGS', 'GET', 'GFLAGS', 'LDFLAGS', 'LDLIBS', 'LEX',
	'LFLAGS', 'LINT', 'LINTFLAGS', 'M2C', 'MAKEINFO', 'PC', 'PFLAGS', 'RFLAGS', 'RM', 'TANGLE', 'TEX',
	'TEXI2DVI', 'WEAVE', 'YACC', 'YFLAGS', --
	'bindir', 'DESTDIR', 'exec_prefix', 'libexecdir', 'prefix', 'sbindir' -- directory
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	-- Filename.
	'abspath', 'addprefix', 'addsuffix', 'basename', 'dir', 'join', 'notdir', 'realpath', 'suffix',
	'wildcard', --
	'and', 'if', 'or', -- conditional
	'error', 'info', 'warning', -- control
	'filter', 'filter-out', 'findstring', 'firstword', 'lastword', 'patsubst', 'sort', 'strip',
	-- Text.
	'subst', 'word', 'wordlist', 'words', --
	'call', 'eval', 'file', 'flavor', 'foreach', 'origin', 'shell', 'value' -- other
})

lexer.property['scintillua.comment'] = '#'

return lex
