-- Copyright 2006-2025 Mitchell. See LICENSE.
-- PHP LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Functions.
local word = (lexer.alpha + '_' + lpeg.R('\127\255')) * (lexer.alnum + '_' + lpeg.R('\127\255'))^0
local func = lex:tag(lexer.FUNCTION, word)
local method = lpeg.B('->') * lex:tag(lexer.FUNCTION_METHOD, word)
lex:add_rule('function', (method + func) * #(lexer.space^0 * '('))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, word))

-- Variables.
lex:add_rule('variable', lex:tag(lexer.VARIABLE, '$' * word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
local heredoc = '<<<' * P(function(input, index)
	local _, e, delimiter = input:find('([%a_][%w_]*)[\n\r\f]+', index)
	if delimiter then
		_, e = input:find('[\n\r\f]+' .. delimiter, e)
		return e and e + 1
	end
end)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str + bq_str + heredoc))
-- TODO: interpolated code.

-- Comments.
local line_comment = lexer.to_eol(P('//') + '#')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('!@%^*&()-+=|/?.,;:<>[]{}')))

-- Embedded in HTML.
local html = lexer.load('html')

-- Embedded PHP.
local php_start_rule = lex:tag(lexer.PREPROCESSOR, '<?' * ('php' * lexer.space)^-1)
local php_end_rule = lex:tag(lexer.PREPROCESSOR, '?>')
html:embed(lex, php_start_rule, php_end_rule)

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, '<?', '?>')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.OPERATOR, '(', ')')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	-- Reserved words (http://php.net/manual/en/reserved.keywords.php)
	'__halt_compiler', 'abstract', 'and', 'array', 'as', 'break', 'callable', 'case', 'catch',
	'class', 'clone', 'const', 'continue', 'declare', 'default', 'die', 'do', 'echo', 'else',
	'elseif', 'empty', 'enddeclare', 'endfor', 'endforeach', 'endif', 'endswitch', 'endwhile', 'eval',
	'exit', 'extends', 'final', 'finally', 'fn', 'for', 'foreach', 'function', 'global', 'goto', 'if',
	'implements', 'include', 'include_once', 'instanceof', 'insteadof', 'interface', 'isset', 'list',
	'namespace', 'new', 'or', 'print', 'private', 'protected', 'public', 'require', 'require_once',
	'return', 'static', 'switch', 'throw', 'trait', 'try', 'unset', 'use', 'var', 'while', 'xor',
	'yield', 'from',
	-- Reserved classes (http://php.net/manual/en/reserved.classes.php)
	'Directory', 'stdClass', '__PHP_Incomplete_Class', 'Exception', 'ErrorException',
	'php_user_filter', 'Closure', 'Generator', 'ArithmeticError', 'AssertionError',
	'DivisionByZeroError', 'Error', 'Throwable', 'ParseError', 'TypeError', 'self', 'static', 'parent'
})

lex:set_word_list(lexer.TYPE, 'int float bool string true false null void iterable object')

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	-- Compile-time (https://www.php.net/manual/en/reserved.keywords.php)
	'__CLASS__', '__DIR__', '__FILE__', '__FUNCTION__', '__LINE__', '__METHOD__', '__NAMESPACE__',
	'__TRAIT__',
	-- Reserved (https://www.php.net/manual/en/reserved.constants.php)
	'PHP_VERSION', 'PHP_MAJOR_VERSION', 'PHP_MINOR_VERSION', 'PHP_RELEASE_VERSION', 'PHP_VERSION_ID',
	'PHP_EXTRA_VERSION', 'PHP_ZTS', 'PHP_DEBUG', 'PHP_MAXPATHLEN', 'PHP_OS', 'PHP_OS_FAMILY',
	'PHP_SAPI', 'PHP_EOL', 'PHP_INT_MAX', 'PHP_INT_MIN', 'PHP_INT_SIZE', 'PHP_FLOAT_DIG',
	'PHP_FLOAT_EPSILON', 'PHP_FLOAT_MIN', 'PHP_FLOAT_MAX', 'DEFAULT_INCLUDE_PATH', 'PEAR_INSTALL_DIR',
	'PEAR_EXTENSION_DIR', 'PHP_EXTENSION_DIR', 'PHP_PREFIX', 'PHP_BINDIR', 'PHP_BINARY', 'PHP_MANDIR',
	'PHP_LIBDIR', 'PHP_DATADIR', 'PHP_SYSCONFDIR', 'PHP_LOCALSTATEDIR', 'PHP_CONFIG_FILE_PATH',
	'PHP_CONFIG_FILE_SCAN_DIR', 'PHP_SHLIB_SUFFIX', 'PHP_FD_SETSIZE', 'E_ERROR', 'E_WARNING',
	'E_PARSE', 'E_NOTICE', 'E_CORE_ERROR', 'E_CORE_WARNING', 'E_COMPILE_ERROR', 'E_USER_ERROR',
	'E_USER_WARNING', 'E_USER_NOTICE', 'E_DEPRECATED', 'E_DEPRECATED', 'E_USER_DEPRECATED', 'E_ALL',
	'E_STRICT', '__COMPILER_HALT_OFFSET__'
})

lexer.property['scintillua.comment'] = '//'

return lex
