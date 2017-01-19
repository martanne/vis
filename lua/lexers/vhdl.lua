-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- VHDL LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'vhdl'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '--' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", true, true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'access', 'after', 'alias', 'all', 'architecture', 'array', 'assert',
  'attribute', 'begin', 'block', 'body', 'buffer', 'bus', 'case', 'component',
  'configuration', 'constant', 'disconnect', 'downto', 'else', 'elsif', 'end',
  'entity', 'exit', 'file', 'for', 'function', 'generate', 'generic', 'group',
  'guarded', 'if', 'impure', 'in', 'inertial', 'inout', 'is', 'label',
  'library', 'linkage', 'literal', 'loop', 'map', 'new', 'next', 'null', 'of',
  'on', 'open', 'others', 'out', 'package', 'port', 'postponed', 'procedure',
  'process', 'pure', 'range', 'record', 'register', 'reject', 'report',
  'return', 'select', 'severity', 'signal', 'shared', 'subtype', 'then', 'to',
  'transport', 'type', 'unaffected', 'units', 'until', 'use', 'variable',
  'wait', 'when', 'while', 'with', 'note', 'warning', 'error', 'failure',
  'and', 'nand', 'or', 'nor', 'xor', 'xnor', 'rol', 'ror', 'sla', 'sll', 'sra',
  'srl', 'mod', 'rem', 'abs', 'not',
  'false', 'true'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'rising_edge', 'shift_left', 'shift_right', 'rotate_left', 'rotate_right',
  'resize', 'std_match', 'to_integer', 'to_unsigned', 'to_signed', 'unsigned',
  'signed', 'to_bit', 'to_bitvector', 'to_stdulogic', 'to_stdlogicvector',
  'to_stdulogicvector'
})

-- Types.
local type = token(l.TYPE, word_match{
  'bit', 'bit_vector', 'character', 'boolean', 'integer', 'real', 'time',
  'string', 'severity_level', 'positive', 'natural', 'signed', 'unsigned',
  'line', 'text', 'std_logic', 'std_logic_vector', 'std_ulogic',
  'std_ulogic_vector', 'qsim_state', 'qsim_state_vector', 'qsim_12state',
  'qsim_12state_vector', 'qsim_strength', 'mux_bit', 'mux_vectory', 'reg_bit',
  'reg_vector', 'wor_bit', 'wor_vector'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'EVENT', 'BASE', 'LEFT', 'RIGHT', 'LOW', 'HIGH', 'ASCENDING', 'IMAGE',
  'VALUE', 'POS', 'VAL', 'SUCC', 'VAL', 'POS', 'PRED', 'VAL', 'POS', 'LEFTOF',
  'RIGHTOF', 'LEFT', 'RIGHT', 'LOW', 'HIGH', 'RANGE', 'REVERSE', 'LENGTH',
  'ASCENDING', 'DELAYED', 'STABLE', 'QUIET', 'TRANSACTION', 'EVENT', 'ACTIVE',
  'LAST', 'LAST', 'LAST', 'DRIVING', 'DRIVING', 'SIMPLE', 'INSTANCE', 'PATH'
})

-- Identifiers.
local word = (l.alpha + "'") * (l.alnum + "_" + "'")^1
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('=/!:;<>+-/*%&|^~()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'type', type},
  {'constant', constant},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
