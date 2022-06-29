-- Copyright 2013-2022 Michael T. Richter. See LICENSE.
-- SNOBOL4 lexer.
-- This lexer works with classic SNOBOL4 as well as the CSNOBOL4 extensions.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local B, P, S = lpeg.B, lpeg.P, lpeg.S

local lex = lexer.new('snobol4')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'ABORT', 'ARRAY', 'CONTINUE', 'DEFINE', 'END', 'FRETURN', 'INPUT', 'NRETURN', 'OUTPUT', 'PUNCH',
  'RETURN', 'SCONTINUE', 'TABLE'
}, true) + '&' * lexer.word))

-- Helper patterns.
local dotted_id = lexer.word * ('.' * lexer.word)^0

-- Labels.
lex:add_rule('label', token(lexer.LABEL, lexer.starts_line(dotted_id)))

-- Targets.
local branch = B(lexer.space * ':(') * dotted_id * #P(')')
local sbranch = B(lexer.space * ':' * S('SsFf') * '(') * dotted_id * #P(')')
local sbranchx = B(')' * S('SsFf') * '(') * dotted_id * #P(')')
lex:add_rule('target', token(lexer.LABEL, branch + sbranch + sbranchx))

-- Patterns.
lex:add_rule('pattern', lexer.token(lexer.CLASS, word_match({
  -- Keep distinct.
  'ABORT', 'ANY', 'ARB', 'ARBNO', 'BAL', 'BREAK', 'BREAKX', 'FAIL', 'FENCE', 'LEN', 'NOTANY', 'POS',
  'REM', 'RPOS', 'RTAB', 'SPAN', 'SUCCEED', 'TAB'
}, true) * #P('(')))

-- Token definitions.
lex:add_rule('built-in', token(lexer.FUNCTION, word_match({
  'APPLY', 'ARRAY', 'CHAR', 'CONVERT', 'COPY', 'DATA', 'DATE', 'DIFFER', 'DUPL', 'EQ', 'EVAL',
  'FILE_ABSPATH', 'FILE_ISDIR', 'FREEZE', 'FUNCTION', 'GE', 'GT', 'HOST', 'IDENT', 'INTEGER',
  'IO_FINDUNIT', 'ITEM', 'LABEL', 'LOAD', 'LPAD', 'LE', 'LGT', 'LT', 'NE', 'OPSYN', 'ORD',
  'PROTOTYPE', 'REMDR', 'REPLACE', 'REVERSE', 'RPAD', 'RSORT', 'SERV_LISTEN', 'SET', 'SETEXIT',
  'SIZE', 'SORT', 'SQRT', 'SSET', 'SUBSTR', 'TABLE', 'THAW', 'TIME', 'TRACE', 'TRIM', 'UNLOAD',
  'VALUE', 'VDIFFER'
}, true) * #P('(')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.DEFAULT, dotted_id))

-- Strings.
local dq_str = lexer.range('"', true, false)
local sq_str = lexer.range("'", true, false)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.starts_line(lexer.to_eol(S('*#|;!')))))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Control.
lex:add_rule('control', token(lexer.PREPROCESSOR, lexer.starts_line('-' * lexer.word)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S'¬?$.!%*/#+-@⊥&^~\\='))

return lex
