-- Copyright 2013-2017 Michael T. Richter. See LICENSE.
-- SNOBOL4 lexer.
-- This lexer works with classic SNOBOL4 as well as the CSNOBOL4 extensions.

local l = require 'lexer'
local token, word_match = l.token, l.word_match
local B, P, R, S, V = lpeg.B, lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = { _NAME = 'snobol4' }

-- Helper patterns.
local dotted_id = l.word * (P'.' * l.word)^0

local dq_str = l.delimited_range('"', true, true)
local sq_str = l.delimited_range("'", true, true)

local branch = B(l.space * P':(') * dotted_id * #P')'
local sbranch = B(l.space * P':' * S'SF' * '(') * dotted_id * #P')'
local sbranchx = B(P')' * S'SF' * P'(') * dotted_id * #P')'

-- Token definitions.
local bif = token(l.FUNCTION, l.word_match({
  'APPLY', 'ARRAY', 'CHAR', 'CONVERT', 'COPY', 'DATA', 'DATE', 'DIFFER', 'DUPL',
  'EQ', 'EVAL', 'FILE_ABSPATH', 'FILE_ISDIR', 'FREEZE', 'FUNCTION', 'GE', 'GT',
  'HOST', 'IDENT', 'INTEGER', 'IO_FINDUNIT', 'ITEM', 'LABEL', 'LOAD', 'LPAD',
  'LE', 'LGT', 'LT', 'NE', 'OPSYN', 'ORD', 'PROTOTYPE', 'REMDR', 'REPLACE',
  'REVERSE', 'RPAD', 'RSORT', 'SERV_LISTEN', 'SET', 'SETEXIT', 'SIZE', 'SORT',
  'SQRT', 'SSET', 'SUBSTR', 'TABLE', 'THAW', 'TIME', 'TRACE', 'TRIM', 'UNLOAD',
  'VALUE', 'VDIFFER',
}, '', true) * #l.delimited_range('()', false, true, true))
local comment = token(l.COMMENT, l.starts_line(S'*#|;!' * l.nonnewline^0))
local control = token(l.PREPROCESSOR, l.starts_line(P'-' * l.word))
local identifier = token(l.DEFAULT, dotted_id)
local keyword = token(l.KEYWORD, l.word_match({
  'ABORT', 'ARRAY', 'CONTINUE', 'DEFINE', 'END', 'FRETURN', 'INPUT', 'NRETURN',
  'OUTPUT', 'PUNCH', 'RETURN', 'SCONTINUE', 'TABLE',
}, '', true) + P'&' * l.word)
local label = token(l.LABEL, l.starts_line(dotted_id))
local number = token(l.NUMBER, l.float + l.integer)
local operator = token(l.OPERATOR, S'¬?$.!%*/#+-@⊥&^~\\=')
local pattern = l.token(l.CLASS, l.word_match({ -- "class" to keep distinct
  'ABORT', 'ANY', 'ARB', 'ARBNO', 'BAL', 'BREAK', 'BREAKX', 'FAIL', 'FENCE',
  'LEN', 'NOTANY', 'POS', 'REM', 'RPOS', 'RTAB', 'SPAN', 'SUCCEED', 'TAB',
}, '', true) * #l.delimited_range('()', false, true, true))
local str = token(l.STRING, sq_str + dq_str)
local target = token(l.LABEL, branch + sbranch + sbranchx)
local ws = token(l.WHITESPACE, l.space^1)

M._rules = {
  { 'comment',    comment     },
  { 'control',    control     },
  { 'string',     str         },
  { 'number',     number      },
  { 'keyword',    keyword     },
  { 'label',      label       },
  { 'target',     target      },
  { 'pattern',    pattern     },
  { 'built-in',   bif         },
  { 'operator',   operator    },
  { 'identifier', identifier  },
  { 'whitespace', ws          },
}

return M
