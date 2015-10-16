-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Batch LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'batch'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local rem = (P('REM') + 'rem') * l.space
local comment = token(l.COMMENT, (rem + '::') * l.nonnewline^0)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'cd', 'chdir', 'md', 'mkdir', 'cls', 'for', 'if', 'echo', 'echo.', 'move',
  'copy', 'ren', 'del', 'set', 'call', 'exit', 'setlocal', 'shift',
  'endlocal', 'pause', 'defined', 'exist', 'errorlevel', 'else', 'in', 'do',
  'NUL', 'AUX', 'PRN', 'not', 'goto', 'pushd', 'popd'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, word_match({
  'APPEND', 'ATTRIB', 'CHKDSK', 'CHOICE', 'DEBUG', 'DEFRAG', 'DELTREE',
  'DISKCOMP', 'DISKCOPY', 'DOSKEY', 'DRVSPACE', 'EMM386', 'EXPAND', 'FASTOPEN',
  'FC', 'FDISK', 'FIND', 'FORMAT', 'GRAPHICS', 'KEYB', 'LABEL', 'LOADFIX',
  'MEM', 'MODE', 'MORE', 'MOVE', 'MSCDEX', 'NLSFUNC', 'POWER', 'PRINT', 'RD',
  'REPLACE', 'RESTORE', 'SETVER', 'SHARE', 'SORT', 'SUBST', 'SYS', 'TREE',
  'UNDELETE', 'UNFORMAT', 'VSAFE', 'XCOPY'
}, nil, true))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local variable = token(l.VARIABLE,
                       '%' * (l.digit + '%' * l.alpha) +
                       l.delimited_range('%', true, true))

-- Operators.
local operator = token(l.OPERATOR, S('+|&!<>='))

-- Labels.
local label = token(l.LABEL, ':' * l.word)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'comment', comment},
  {'identifier', identifier},
  {'string', string},
  {'variable', variable},
  {'label', label},
  {'operator', operator},
}

M._LEXBYLINE = true

M._foldsymbols = {
  _patterns = {'[A-Za-z]+'},
  [l.KEYWORD] = {setlocal = 1, endlocal = -1, SETLOCAL = 1, ENDLOCAL = -1}
}

return M
