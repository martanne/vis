-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Perl LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Markers.
lex:add_rule('marker', lex:tag(lexer.COMMENT, lexer.word_match('__DATA__ __END__') * lexer.any^0))

-- Strings.
local delimiter_matches = {['('] = ')', ['['] = ']', ['{'] = '}', ['<'] = '>'}
local literal_delimited = P(function(input, index) -- for single delimiter sets
	local delimiter = input:sub(index, index)
	if not delimiter:find('%w') then -- only non alpha-numerics
		local patt
		if delimiter_matches[delimiter] then
			-- Handle nested delimiter/matches in strings.
			local s, e = delimiter, delimiter_matches[delimiter]
			patt = lexer.range(s, e, false, true, true)
		else
			patt = lexer.range(delimiter)
		end
		local match_pos = lpeg.match(patt, input, index)
		return match_pos or #input + 1
	end
end)
local literal_delimited2 = P(function(input, index) -- for 2 delimiter sets
	local delimiter = input:sub(index, index)
	-- Only consider non-alpha-numerics and non-spaces as delimiters. The non-spaces are used to
	-- ignore operators like "-s".
	if not delimiter:find('[%w ]') then
		local patt
		if delimiter_matches[delimiter] then
			-- Handle nested delimiter/matches in strings.
			local s, e = delimiter, delimiter_matches[delimiter]
			patt = lexer.range(s, e, false, true, true)
		else
			patt = lexer.range(delimiter)
		end
		local first_match_pos = lpeg.match(patt, input, index)
		local final_match_pos = lpeg.match(patt, input, first_match_pos - 1)
		if not final_match_pos then -- using (), [], {}, or <> notation
			final_match_pos = lpeg.match(lexer.space^0 * patt, input, first_match_pos)
		end
		if final_match_pos and final_match_pos < index then final_match_pos = index end
		return final_match_pos or #input + 1
	end
end)

local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local cmd_str = lexer.range('`')
local heredoc = '<<' * P(function(input, index)
	local s, e, delimiter = input:find('([%a_][%w_]*)[\n\r\f;]+', index)
	if s == index and delimiter then
		local end_heredoc = '[\n\r\f]+'
		e = select(2, input:find(end_heredoc .. delimiter, e))
		return e and e + 1 or #input + 1
	end
end)
local lit_str = 'q' * P('q')^-1 * literal_delimited
local lit_array = 'qw' * literal_delimited
local lit_cmd = 'qx' * literal_delimited
local string = lex:tag(lexer.STRING,
	sq_str + dq_str + cmd_str + heredoc + lit_str + lit_array + lit_cmd)
local regex_str = lexer.after_set('-<>+*!~\\=%&|^?:;([{', lexer.range('/', true) * S('imosx')^0)
local lit_regex = 'qr' * literal_delimited * S('imosx')^0
local lit_match = 'm' * literal_delimited * S('cgimosx')^0
local lit_sub = 's' * literal_delimited2 * S('ecgimosx')^0
local lit_tr = (P('tr') + 'y') * literal_delimited2 * S('cds')^0
local regex = lex:tag(lexer.REGEX, regex_str + lit_regex + lit_match + lit_sub + lit_tr)
lex:add_rule('string', string + regex)

-- Functions.
lex:add_rule('function_builtin',
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN)) *
		#(lexer.space^0 * P('(')^-1))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = lpeg.B('->') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (method + func) * #(lexer.space^0 * '('))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('#', true)
local block_comment = lexer.range(lexer.starts_line('=' * lexer.alpha), lexer.starts_line('=cut'))
lex:add_rule('comment', lex:tag(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_')))

-- Variables.
local builtin_var_s = '$' *
	(lpeg.R('09') + S('!"$%&\'()+,-./:;<=>?@\\]_`|~') + '^' * S('ACDEFHILMNOPRSTVWX')^-1 + 'ARGV')
local builtin_var_a = '@' * (S('+-_F') + 'ARGV' + 'INC' + 'ISA')
local builtin_var_h = '%' * (S('+-!') + '^' * S('H')^-1 + 'ENV' + 'INC' + 'SIG')
lex:add_rule('variable_builtin',
	lex:tag(lexer.VARIABLE_BUILTIN, builtin_var_s + builtin_var_a + builtin_var_h))
local special_var = '$' *
	('^' * S('ADEFHILMOPSTWX')^-1 + S('\\"[]\'&`+*.,;=%~?@<>(|/!-') + ':' * (lexer.any - ':') +
		(P('$') * -lexer.word) + lexer.digit^1)
local plain_var = ('$#' + S('$@%')) * P('$')^0 * lexer.word + '$#'
lex:add_rule('variable', lex:tag(lexer.VARIABLE, special_var + plain_var))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '..' + S('-<>+*!~\\=/%&|^.,?:;()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'STDIN', 'STDOUT', 'STDERR', 'BEGIN', 'END', 'CHECK', 'INIT', --
	'require', 'use', --
	'break', 'continue', 'do', 'each', 'else', 'elsif', 'foreach', 'for', 'if', 'last', 'local', 'my',
	'next', 'our', 'package', 'return', 'sub', 'unless', 'until', 'while', '__FILE__', '__LINE__',
	'__PACKAGE__', --
	'and', 'or', 'not', 'eq', 'ne', 'lt', 'gt', 'le', 'ge'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'abs', 'accept', 'alarm', 'atan2', 'bind', 'binmode', 'bless', 'caller', 'chdir', 'chmod',
	'chomp', 'chop', 'chown', 'chr', 'chroot', 'closedir', 'close', 'connect', 'cos', 'crypt',
	'dbmclose', 'dbmopen', 'defined', 'delete', 'die', 'dump', 'each', 'endgrent', 'endhostent',
	'endnetent', 'endprotoent', 'endpwent', 'endservent', 'eof', 'eval', 'exec', 'exists', 'exit',
	'exp', 'fcntl', 'fileno', 'flock', 'fork', 'format', 'formline', 'getc', 'getgrent', 'getgrgid',
	'getgrnam', 'gethostbyaddr', 'gethostbyname', 'gethostent', 'getlogin', 'getnetbyaddr',
	'getnetbyname', 'getnetent', 'getpeername', 'getpgrp', 'getppid', 'getpriority', 'getprotobyname',
	'getprotobynumber', 'getprotoent', 'getpwent', 'getpwnam', 'getpwuid', 'getservbyname',
	'getservbyport', 'getservent', 'getsockname', 'getsockopt', 'glob', 'gmtime', 'goto', 'grep',
	'hex', 'import', 'index', 'int', 'ioctl', 'join', 'keys', 'kill', 'lcfirst', 'lc', 'length',
	'link', 'listen', 'localtime', 'log', 'lstat', 'map', 'mkdir', 'msgctl', 'msgget', 'msgrcv',
	'msgsnd', 'new', 'oct', 'opendir', 'open', 'ord', 'pack', 'pipe', 'pop', 'pos', 'printf', 'print',
	'prototype', 'push', 'quotemeta', 'rand', 'readdir', 'read', 'readlink', 'recv', 'redo', 'ref',
	'rename', 'reset', 'reverse', 'rewinddir', 'rindex', 'rmdir', 'scalar', 'seekdir', 'seek',
	'select', 'semctl', 'semget', 'semop', 'send', 'setgrent', 'sethostent', 'setnetent', 'setpgrp',
	'setpriority', 'setprotoent', 'setpwent', 'setservent', 'setsockopt', 'shift', 'shmctl', 'shmget',
	'shmread', 'shmwrite', 'shutdown', 'sin', 'sleep', 'socket', 'socketpair', 'sort', 'splice',
	'split', 'sprintf', 'sqrt', 'srand', 'stat', 'study', 'substr', 'symlink', 'syscall', 'sysread',
	'sysseek', 'system', 'syswrite', 'telldir', 'tell', 'tied', 'tie', 'time', 'times', 'truncate',
	'ucfirst', 'uc', 'umask', 'undef', 'unlink', 'unpack', 'unshift', 'untie', 'utime', 'values',
	'vec', 'wait', 'waitpid', 'wantarray', 'warn', 'write'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'ARGV', 'ARGVOUT', 'DATA', 'ENV', 'INC', 'SIG', 'STDERR', 'STDIN', 'STDOUT'
})

lexer.property['scintillua.comment'] = '#'

return lex
