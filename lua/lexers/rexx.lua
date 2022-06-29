-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Rexx LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rexx')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'address', 'arg', 'by', 'call', 'class', 'do', 'drop', 'else', 'end', 'exit', 'expose', 'forever',
  'forward', 'guard', 'if', 'interpret', 'iterate', 'leave', 'method', 'nop', 'numeric',
  'otherwise', 'parse', 'procedure', 'pull', 'push', 'queue', 'raise', 'reply', 'requires',
  'return', 'routine', 'result', 'rc', 'say', 'select', 'self', 'sigl', 'signal', 'super', 'then',
  'to', 'trace', 'use', 'when', 'while', 'until'
}, true)))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match({
  'abbrev', 'abs', 'address', 'arg', 'beep', 'bitand', 'bitor', 'bitxor', 'b2x', 'center',
  'changestr', 'charin', 'charout', 'chars', 'compare', 'consition', 'copies', 'countstr', 'c2d',
  'c2x', 'datatype', 'date', 'delstr', 'delword', 'digits', 'directory', 'd2c', 'd2x', 'errortext',
  'filespec', 'form', 'format', 'fuzz', 'insert', 'lastpos', 'left', 'length', 'linein', 'lineout',
  'lines', 'max', 'min', 'overlay', 'pos', 'queued', 'random', 'reverse', 'right', 'sign',
  'sourceline', 'space', 'stream', 'strip', 'substr', 'subword', 'symbol', 'time', 'trace',
  'translate', 'trunc', 'value', 'var', 'verify', 'word', 'wordindex', 'wordlength', 'wordpos',
  'words', 'xrange', 'x2b', 'x2c', 'x2d', --
  'rxfuncadd', 'rxfuncdrop', 'rxfuncquery', 'rxmessagebox', 'rxwinexec', 'sysaddrexxmacro',
  'sysbootdrive', 'sysclearrexxmacrospace', 'syscloseeventsem', 'sysclosemutexsem', 'syscls',
  'syscreateeventsem', 'syscreatemutexsem', 'syscurpos', 'syscurstate', 'sysdriveinfo',
  'sysdrivemap', 'sysdropfuncs', 'sysdroprexxmacro', 'sysdumpvariables', 'sysfiledelete',
  'sysfilesearch', 'sysfilesystemtype', 'sysfiletree', 'sysfromunicode', 'systounicode',
  'sysgeterrortext', 'sysgetfiledatetime', 'sysgetkey', 'sysini', 'sysloadfuncs',
  'sysloadrexxmacrospace', 'sysmkdir', 'sysopeneventsem', 'sysopenmutexsem', 'sysposteventsem',
  'syspulseeventsem', 'sysqueryprocess', 'sysqueryrexxmacro', 'sysreleasemutexsem',
  'sysreorderrexxmacro', 'sysrequestmutexsem', 'sysreseteventsem', 'sysrmdir',
  'syssaverexxmacrospace', 'syssearchpath', 'syssetfiledatetime', 'syssetpriority', 'syssleep',
  'sysstemcopy', 'sysstemdelete', 'syssteminsert', 'sysstemsort', 'sysswitchsession',
  'syssystemdirectory', 'systempfilename', 'systextscreenread', 'systextscreensize',
  'sysutilversion', 'sysversion', 'sysvolumelabel', 'syswaiteventsem', 'syswaitnamedpipe',
  'syswindecryptfile', 'syswinencryptfile', 'syswinver'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.alpha * (lexer.alnum + S('@#$\\.!?_'))^0))

-- Strings.
local sq_str = lexer.range("'", true, false)
local dq_str = lexer.range('"', true, false)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('--', true)
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Preprocessor.
lex:add_rule('preprocessor', token(lexer.PREPROCESSOR, lexer.to_eol(lexer.starts_line('#'))))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=!<>+-/\\*%&|^~.,:;(){}')))

-- Fold points
lex:add_fold_point(lexer.KEYWORD, 'do', 'end')
lex:add_fold_point(lexer.KEYWORD, 'select', 'return')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('--'))
-- lex:add_fold_point(lexer.OPERATOR, ':', ?)

return lex
