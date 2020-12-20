-- Copyright 2013-2020 Michael T. Richter. See LICENSE.
-- SNOBOL4 lexer.
-- This lexer works with classic SNOBOL4 as well as the CSNOBOL4 extensions.

local lexer = require 'lexer'
local token, word_match = lexer.token, lexer.word_match
local B, P, S, V = lpeg.B, lpeg.P, lpeg.S, lpeg.V

local M = { _NAME = 'snobol4' }

-- Helper patterns.
local dotted_id = lexer.word * (P'.' * lexer.word)^0

local dq_str = lexer.range('"', true, false)
local sq_str = lexer.range("'", true, false)

local branch = B(lexer.space * P':(') * dotted_id * #P')'
local sbranch = B(lexer.space * P':' * S'SF' * '(') * dotted_id * #P')'
local sbranchx = B(P')' * S'SF' * P'(') * dotted_id * #P')'

-- Token definitions.
local bif = token(lexer.FUNCTION, word_match({
  'APPLY', 'ARRAY', 'CHAR', 'CONVERT', 'COPY', 'DATA', 'DATE', 'DIFFER', 'DUPL',
  'EQ', 'EVAL', 'FILE_ABSPATH', 'FILE_ISDIR', 'FREEZE', 'FUNCTION', 'GE', 'GT',
  'HOST', 'IDENT', 'INTEGER', 'IO_FINDUNIT', 'ITEM', 'LABEL', 'LOAD', 'LPAD',
  'LE', 'LGT', 'LT', 'NE', 'OPSYN', 'ORD', 'PROTOTYPE', 'REMDR', 'REPLACE',
  'REVERSE', 'RPAD', 'RSORT', 'SERV_LISTEN', 'SET', 'SETEXIT', 'SIZE', 'SORT',
  'SQRT', 'SSET', 'SUBSTR', 'TABLE', 'THAW', 'TIME', 'TRACE', 'TRIM', 'UNLOAD',
  'VALUE', 'VDIFFER',
}, '', true) * #lexer.range('(', ')', false, false, true))
local comment = token(lexer.COMMENT, lexer.starts_line(S'*#|;!' *
  lexer.nonnewline^0))
local control = token(lexer.PREPROCESSOR, lexer.starts_line(P'-' * lexer.word))
local identifier = token(lexer.DEFAULT, dotted_id)
local keyword = token(lexer.KEYWORD, word_match({
  'ABORT', 'ARRAY', 'CONTINUE', 'DEFINE', 'END', 'FRETURN', 'INPUT', 'NRETURN',
  'OUTPUT', 'PUNCH', 'RETURN', 'SCONTINUE', 'TABLE',
}, '', true) + P'&' * lexer.word)
local label = token(lexer.LABEL, lexer.starts_line(dotted_id))
local number = token(lexer.NUMBER, lexer.float + lexer.integer)
local operator = token(lexer.OPERATOR, S'¬?$.!%*/#+-@⊥&^~\\=')
local pattern = lexer.token(lexer.CLASS, word_match({ -- keep distinct
  'ABORT', 'ANY', 'ARB', 'ARBNO', 'BAL', 'BREAK', 'BREAKX', 'FAIL', 'FENCE',
  'LEN', 'NOTANY', 'POS', 'REM', 'RPOS', 'RTAB', 'SPAN', 'SUCCEED', 'TAB',
}, '', true) * #lexer.range('(', ')', false, false, true))
local str = token(lexer.STRING, sq_str + dq_str)
local target = token(lexer.LABEL, branch + sbranch + sbranchx)
local ws = token(lexer.WHITESPACE, lexer.space^1)

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
