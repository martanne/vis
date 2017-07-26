-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- PHP LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = {_NAME = 'php'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = (P('//') + '#') * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, block_comment + line_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local bt_str = l.delimited_range('`')
local heredoc = '<<<' * P(function(input, index)
  local _, e, delimiter = input:find('([%a_][%w_]*)[\n\r\f]+', index)
  if delimiter then
    local _, e = input:find('[\n\r\f]+'..delimiter, e)
    return e and e + 1
  end
end)
local string = token(l.STRING, sq_str + dq_str + bt_str + heredoc)
-- TODO: interpolated code.

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
-- keywords
-- http://php.net/manual/en/reserved.keywords.php
	'__halt_compiler', 'abstract', 'and', 'array', 'as', 'break',
	'callable', 'case', 'catch', 'class', 'clone', 'const', 'continue',
	'declare', 'default', 'die', 'do', 'echo', 'else', 'elseif', 'empty',
	'enddeclare', 'endfor', 'endforeach', 'endif', 'endswitch',
	'endwhile', 'eval', 'exit', 'extends', 'final', 'for', 'foreach',
	'function', 'global', 'goto', 'if', 'implements', 'include',
	'list', 'namespace', 'new', 'or', 'print', 'private', 'protected',
	'public', 'require', 'require_once', 'return', 'static',
	'switch', 'throw', 'trait', 'try', 'unset', 'use', 'var', 'while', 'xor',
-- predefined _classes 
-- http://php.net/manual/en/reserved.classes.php
	'directory', 'stdclass', '__php_incomplete_class', 'exception', 'errorexception',
	'closure', 'generator', 'arithmeticerror', 'assertionerror', 'divisionbyzeroerror', 'error', 
	'throwable', 'parseerror', 'typeerror', 'self', 'parent',
-- other_reserved 
-- http://php.net/manual/en/reserved.other-reserved-words.php
	'int', 'float', 'bool', 'string', 'true', 'false', 'null', 'void', 'iterable', 'resource',
	'object', 'mixed', 'numeric'
})

-- Variables.
local word = (l.alpha + '_' + R('\127\255')) * (l.alnum + '_' + R('\127\255'))^0
local variable = token(l.VARIABLE, '$' * word)

-- Identifiers.
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('!@%^*&()-+=|/.,;:<>[]{}') + '?' * -P('>'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'variable', variable},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

-- Embedded in HTML.
local html = l.load('html')

-- Embedded PHP.
local php_start_rule = token('php_tag', '<?' * ('php' * l.space)^-1)
local php_end_rule = token('php_tag', '?>')
l.embed_lexer(html, M, php_start_rule, php_end_rule)

M._tokenstyles = {
  php_tag = l.STYLE_EMBEDDED
}

local _foldsymbols = html._foldsymbols
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '<%?'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '%?>'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '/%*'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '%*/'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '//'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '#'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '[{}()]'
_foldsymbols.php_tag = {['<?'] = 1, ['?>'] = -1}
_foldsymbols[l.COMMENT]['/*'], _foldsymbols[l.COMMENT]['*/'] = 1, -1
_foldsymbols[l.COMMENT]['//'] = l.fold_line_comments('//')
_foldsymbols[l.COMMENT]['#'] = l.fold_line_comments('#')
_foldsymbols[l.OPERATOR] = {['{'] = 1, ['}'] = -1, ['('] = 1, [')'] = -1}
M._foldsymbols = _foldsymbols

return M
