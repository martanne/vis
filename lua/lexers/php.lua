-- Copyright 2006-2022 Mitchell. See LICENSE.
-- PHP LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('php')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
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
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE,
  word_match('int float bool string true false null void iterable object')))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
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
}))

-- Identifiers.
local word = (lexer.alpha + '_' + lpeg.R('\127\255')) * (lexer.alnum + '_' + lpeg.R('\127\255'))^0
lex:add_rule('identifier', token(lexer.IDENTIFIER, word))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, '$' * word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
local heredoc = '<<<' * P(function(input, index)
  local _, e, delimiter = input:find('([%a_][%w_]*)[\n\r\f]+', index)
  if delimiter then
    e = select(2, input:find('[\n\r\f]+' .. delimiter, e))
    return e and e + 1
  end
end)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + bq_str + heredoc))
-- TODO: interpolated code.

-- Comments.
local line_comment = lexer.to_eol(P('//') + '#')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!@%^*&()-+=|/?.,;:<>[]{}')))

-- Embedded in HTML.
local html = lexer.load('html')

-- Embedded PHP.
local php_start_rule = token('php_tag', '<?' * ('php' * lexer.space)^-1)
local php_end_rule = token('php_tag', '?>')
html:embed(lex, php_start_rule, php_end_rule)
lex:add_style('php_tag', lexer.styles.embedded)

-- Fold points.
lex:add_fold_point('php_tag', '<?', '?>')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.OPERATOR, '(', ')')

return lex
