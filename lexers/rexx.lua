-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Rexx LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'rexx'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '--' * l.nonnewline_esc^0
local block_comment = l.nested_pair('/*', '*/')
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Preprocessor.
local preproc = token(l.PREPROCESSOR, l.starts_line('#') * l.nonnewline^0)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'address', 'arg', 'by', 'call', 'class', 'do', 'drop', 'else', 'end', 'exit',
  'expose', 'forever', 'forward', 'guard', 'if', 'interpret', 'iterate',
  'leave', 'method', 'nop', 'numeric', 'otherwise', 'parse', 'procedure',
  'pull', 'push', 'queue', 'raise', 'reply', 'requires', 'return', 'routine',
  'result', 'rc', 'say', 'select', 'self', 'sigl', 'signal', 'super', 'then',
  'to', 'trace', 'use', 'when', 'while', 'until'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, word_match({
  'abbrev', 'abs', 'address', 'arg', 'beep', 'bitand', 'bitor', 'bitxor', 'b2x',
  'center', 'changestr', 'charin', 'charout', 'chars', 'compare', 'consition',
  'copies', 'countstr', 'c2d', 'c2x', 'datatype', 'date', 'delstr', 'delword',
  'digits', 'directory', 'd2c', 'd2x', 'errortext', 'filespec', 'form',
  'format', 'fuzz', 'insert', 'lastpos', 'left', 'length', 'linein', 'lineout',
  'lines', 'max', 'min', 'overlay', 'pos', 'queued', 'random', 'reverse',
  'right', 'sign', 'sourceline', 'space', 'stream', 'strip', 'substr',
  'subword', 'symbol', 'time', 'trace', 'translate', 'trunc', 'value', 'var',
  'verify', 'word', 'wordindex', 'wordlength', 'wordpos', 'words', 'xrange',
  'x2b', 'x2c', 'x2d', 'rxfuncadd', 'rxfuncdrop', 'rxfuncquery', 'rxmessagebox',
  'rxwinexec', 'sysaddrexxmacro', 'sysbootdrive', 'sysclearrexxmacrospace',
  'syscloseeventsem', 'sysclosemutexsem', 'syscls', 'syscreateeventsem',
  'syscreatemutexsem', 'syscurpos', 'syscurstate', 'sysdriveinfo',
  'sysdrivemap', 'sysdropfuncs', 'sysdroprexxmacro', 'sysdumpvariables',
  'sysfiledelete', 'sysfilesearch', 'sysfilesystemtype', 'sysfiletree',
  'sysfromunicode', 'systounicode', 'sysgeterrortext', 'sysgetfiledatetime',
  'sysgetkey', 'sysini', 'sysloadfuncs', 'sysloadrexxmacrospace', 'sysmkdir',
  'sysopeneventsem', 'sysopenmutexsem', 'sysposteventsem', 'syspulseeventsem',
  'sysqueryprocess', 'sysqueryrexxmacro', 'sysreleasemutexsem',
  'sysreorderrexxmacro', 'sysrequestmutexsem', 'sysreseteventsem', 'sysrmdir',
  'syssaverexxmacrospace', 'syssearchpath', 'syssetfiledatetime',
  'syssetpriority', 'syssleep', 'sysstemcopy', 'sysstemdelete', 'syssteminsert',
  'sysstemsort', 'sysswitchsession', 'syssystemdirectory', 'systempfilename',
  'systextscreenread', 'systextscreensize', 'sysutilversion', 'sysversion',
  'sysvolumelabel', 'syswaiteventsem', 'syswaitnamedpipe', 'syswindecryptfile',
  'syswinencryptfile', 'syswinver'
}, '2', true))

-- Identifiers.
local word = l.alpha * (l.alnum + S('@#$\\.!?_')^0)
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('=!<>+-/\\*%&|^~.,:;(){}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '/%*', '%*/', '%-%-', ':'},
  [l.KEYWORD] = {['do'] = 1, select = 1, ['end'] = -1, ['return'] = -1},
  [l.COMMENT] = {
    ['/*'] = 1, ['*/'] = -1, ['--'] = l.fold_line_comments('--')
  },
  [l.OPERATOR] = {[':'] = 1}
}

return M
