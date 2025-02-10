-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Shell LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Builtins.
lex:add_rule('builtin',
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN)) * -P('='))

-- Variable assignment.
local assign = lex:tag(lexer.VARIABLE, lexer.word) * lex:tag(lexer.OPERATOR, '=')
lex:add_rule('assign', lexer.starts_line(assign, true))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = -B('\\') * lexer.range("'", false, false)
local dq_str = -B('\\') * lexer.range('"')
local heredoc = '<<' * P(function(input, index)
	local _, e, minus, _, delimiter = input:find('^(%-?)%s*(["\']?)([%w_]+)%2[^\n]*[\n\r\f;]+', index)
	if not delimiter then return nil end
	-- If the starting delimiter of a here-doc begins with "-", then spaces are allowed to come
	-- before the closing delimiter.
	_, e =
		input:find((minus == '' and '[\n\r\f]+' or '[\n\r\f]+[ \t]*') .. delimiter .. '%f[^%w_]', e)
	return e and e + 1 or #input + 1
end)
local ex_str = -B('\\') * '`'
lex:add_rule('string',
	lex:tag(lexer.STRING, sq_str + dq_str + heredoc) + lex:tag(lexer.EMBEDDED, ex_str))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, -B('\\') * lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Variables.
local builtin_var = lex:tag(lexer.OPERATOR, '$' * P('{')^-1) * lex:tag(lexer.VARIABLE_BUILTIN,
	lex:word_match(lexer.VARIABLE_BUILTIN) + S('!#?*@$-') * -lexer.alnum + lexer.digit^1)
local var_ref = lex:tag(lexer.OPERATOR, '$' * ('{' * S('!#')^-1)^-1) *
	lex:tag(lexer.VARIABLE, lexer.word)
local patt_expansion = lex:tag(lexer.DEFAULT, '/#' + '#' * P('#')^-1)
lex:add_rule('variable', builtin_var + var_ref * patt_expansion^-1)

-- Operators.
local op = S('!<>&|;$()[]{}') + lpeg.B(lexer.space) * S('.:') * #lexer.space

local function in_expr(constructs)
	return P(function(input, index)
		local line = input:sub(1, index):match('[^\r\n]*$')
		for k, v in pairs(constructs) do
			local s = line:find(k, 1, true)
			if not s then goto continue end
			local e = line:find(v, 1, true)
			if not e or e < s then return true end
			::continue::
		end
		return nil
	end)
end

local file_op = '-' * (S('abcdefghkprstuwxGLNOS') + 'ef' + 'nt' + 'ot')
local shell_op = '-o'
local var_op = '-' * S('vR')
local string_op = '-' * S('zn') + S('!=')^-1 * '=' + S('<>')
local num_op = '-' * lexer.word_match('eq ne lt le gt ge')
local in_cond_expr = in_expr{['[[ '] = ' ]]', ['[ '] = ' ]'}
local conditional_op = (num_op + file_op + shell_op + var_op + string_op) * #lexer.space *
	in_cond_expr

local in_arith_expr = in_expr{['(('] = '))'}
local arith_op = (S('+!~*/%<>=&^|?:,') + '--' + '-' * #S(' \t')) * in_arith_expr

-- TODO: performance is terrible on large files.
-- lex:add_rule('operator', lex:tag(lexer.OPERATOR, op + conditional_op + arith_op))
lex:add_rule('operator', lex:tag(lexer.OPERATOR, op))

-- Flags/options.
lex:add_rule('flag', lex:tag(lexer.DEFAULT, '-' * P('-')^-1 * lexer.word * ('-' * lexer.word)^0))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'if', 'fi')
lex:add_fold_point(lexer.KEYWORD, 'case', 'esac')
lex:add_fold_point(lexer.KEYWORD, 'do', 'done')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'if', 'then', 'elif', 'else', 'fi', 'time', 'for', 'in', 'until', 'while', 'do', 'done', 'case',
	'esac', 'coproc', 'select', 'function'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	-- Shell built-ins.
	'break', 'cd', 'continue', 'eval', 'exec', 'exit', 'export', 'getopts', 'hash', 'pwd', 'readonly',
	'return', 'shift', 'test', 'times', 'trap', 'umask', 'unset',
	-- Bash built-ins.
	'alias', 'bind', 'builtin', 'caller', 'command', 'declare', 'echo', 'enable', 'help', 'let',
	'local', 'logout', 'mapfile', 'printf', 'read', 'readarray', 'source', 'type', 'typeset',
	'ulimit', 'unalias', --
	'set', 'shopt', -- shell behavior
	'dirs', 'popd', 'pushd', -- directory stack
	'bg', 'fg', 'jobs', 'kill', 'wait', 'disown', 'suspend', -- job control
	'fc', 'history' -- history
})

lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	-- Shell built-ins.
	'CDPATH', 'HOME', 'IFS', 'MAIL', 'MAILPATH', 'OPTARG', 'OPTIND', 'PATH', 'PS1', 'PS2',
	-- Bash built-ins.
	'BASH', 'BASHOPTS', 'BASHPID', 'BASH_ALIASES', 'BASH_ARGC', 'BASH_ARGV', 'BASH_ARGV0',
	'BASH_CMDS', 'BASH_COMMAND', 'BASH_COMPAT', 'BASH_ENV', 'BASH_EXECUTION_STRING', 'BASH_LINENO',
	'BASH_LOADABLES_PATH', 'BASH_REMATCH', 'BASH_SOURCE', 'BASH_SUBSHELL', 'BASH_VERSINFO',
	'BASH_VERSION', 'BASH_XTRACEFD', 'CHILD_MAX', 'COLUMNS', 'COMP_CWORD', 'COMP_LINE', 'COMP_POINT',
	'COMP_TYPE', 'COMP_KEY', 'COMP_WORDBREAKS', 'COMP_WORDS', 'COMP_REPLY', 'COPROC', 'DIRSTACK',
	'EMACS', 'ENV', 'EPOCHREALTIME', 'EPOCHSECONDS', 'EUID', 'EXECIGNORE', 'FCEDIT', 'FIGNORE',
	'FUNCNAME', 'FUNCNEST', 'GLOBIGNORE', 'GROUPS', 'histchars', 'HISTCMD', 'HISTCONTROL', 'HISTFILE',
	'HISTFILESIZE', 'HISTIGNORE', 'HISTSIZE', 'HISTTIMEFORMAT', 'HOSTFILE', 'HOSTNAME', 'HOSTTYPE',
	'IGNOREEOF', 'INPUTRC', 'INSIDE_EMACS', 'LANG', 'LC_ALL', 'LC_COLLATE', 'LC_CTYPE', 'LC_MESSAGES',
	'LC_NUMERIC', 'LC_TIME', 'LINENO', 'LINES', 'MACHTYPE', 'MAILCHECK', 'MAPFILE', 'OLDPWD',
	'OPTERR', 'OSTYPE', 'PIPESTATUS', 'POSIXLY_CORRECT', 'PPID', 'PROMPT_COMMAND', 'PROMPT_DIRTRIM',
	'PSO', 'PS3', 'PS4', 'PWD', 'RANDOM', 'READLINE_LINE', 'READLINE_MARK', 'READLINE_POINT', 'REPLY',
	'SECONDS', 'SHELL', 'SHELLOPTS', 'SHLVL', 'SRANDOM', 'TIMEFORMAT', 'TMOUT', 'TMPDIR', 'UID',
	-- Job control.
	'auto_resume'
})

lexer.property['scintillua.comment'] = '#'

return lex
