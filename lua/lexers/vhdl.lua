-- Copyright 2006-2022 Mitchell. See LICENSE.
-- VHDL LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('vhdl')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'access', 'after', 'alias', 'all', 'architecture', 'array', 'assert', 'attribute', 'begin',
  'block', 'body', 'buffer', 'bus', 'case', 'component', 'configuration', 'constant', 'disconnect',
  'downto', 'else', 'elsif', 'end', 'entity', 'exit', 'file', 'for', 'function', 'generate',
  'generic', 'group', 'guarded', 'if', 'impure', 'in', 'inertial', 'inout', 'is', 'label',
  'library', 'linkage', 'literal', 'loop', 'map', 'new', 'next', 'null', 'of', 'on', 'open',
  'others', 'out', 'package', 'port', 'postponed', 'procedure', 'process', 'pure', 'range',
  'record', 'register', 'reject', 'report', 'return', 'select', 'severity', 'signal', 'shared',
  'subtype', 'then', 'to', 'transport', 'type', 'unaffected', 'units', 'until', 'use', 'variable',
  'wait', 'when', 'while', 'with', --
  'note', 'warning', 'error', 'failure', --
  'and', 'nand', 'or', 'nor', 'xor', 'xnor', 'rol', 'ror', 'sla', 'sll', 'sra', 'srl', 'mod', 'rem', --
  'abs', 'not', 'false', 'true'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'rising_edge', 'shift_left', 'shift_right', 'rotate_left', 'rotate_right', 'resize', 'std_match',
  'to_integer', 'to_unsigned', 'to_signed', 'unsigned', 'signed', 'to_bit', 'to_bitvector',
  'to_stdulogic', 'to_stdlogicvector', 'to_stdulogicvector'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bit', 'bit_vector', 'character', 'boolean', 'integer', 'real', 'time', 'string',
  'severity_level', 'positive', 'natural', 'signed', 'unsigned', 'line', 'text', 'std_logic',
  'std_logic_vector', 'std_ulogic', 'std_ulogic_vector', 'qsim_state', 'qsim_state_vector',
  'qsim_12state', 'qsim_12state_vector', 'qsim_strength', 'mux_bit', 'mux_vectory', 'reg_bit',
  'reg_vector', 'wor_bit', 'wor_vector'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
  'EVENT', 'BASE', 'LEFT', 'RIGHT', 'LOW', 'HIGH', 'ASCENDING', 'IMAGE', 'VALUE', 'POS', 'VAL',
  'SUCC', 'VAL', 'POS', 'PRED', 'VAL', 'POS', 'LEFTOF', 'RIGHTOF', 'LEFT', 'RIGHT', 'LOW', 'HIGH',
  'RANGE', 'REVERSE', 'LENGTH', 'ASCENDING', 'DELAYED', 'STABLE', 'QUIET', 'TRANSACTION', 'EVENT',
  'ACTIVE', 'LAST', 'LAST', 'LAST', 'DRIVING', 'DRIVING', 'SIMPLE', 'INSTANCE', 'PATH'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + "'") * (lexer.alnum + S("_'"))^1))

-- Strings.
local sq_str = lexer.range("'", true, false)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('--')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=/!:;<>+-/*%&|^~()')))

return lex
