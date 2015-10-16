-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Prolog LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'prolog'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '%' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.digit^1 * ('.' * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'module', 'meta_predicate', 'multifile', 'dynamic', 'abolish',
  'current_output', 'peek_code', 'append', 'current_predicate', 'put_byte',
  'arg', 'current_prolog_flag', 'put_char', 'asserta', 'assert', 'fail',
  'put_code', 'assertz', 'findall', 'read', 'at_end_of_stream', 'float',
  'read_term', 'atom', 'flush_output', 'repeat', 'atom_chars', 'functor',
  'retract', 'atom_codes', 'get_byte', 'set_input', 'atom_concat', 'get_char',
  'set_output', 'atom_length', 'get_code', 'set_prolog_flag', 'atomic', 'halt',
  'set_stream_position', 'bagof', 'integer', 'setof', 'call', 'is',
  'stream_property', 'catch', 'nl', 'sub_atom', 'char_code', 'nonvar', 'throw',
  'char_conversion', 'number', 'clause', 'number_chars',
  'unify_with_occurs_check', 'close', 'number_codes', 'var', 'compound', 'once',
  'copy_term', 'op', 'write', 'writeln', 'write_canonical', 'write_term',
  'writeq', 'current_char_conversion', 'open', 'current_input', 'peek_byte',
  'current_op', 'peek_char', 'false', 'true', 'consult', 'member', 'memberchk',
  'reverse', 'permutation', 'delete',
  -- Math.
  'mod', 'div', 'abs', 'exp', 'ln', 'log', 'sqrt', 'round', 'trunc', 'val',
  'cos', 'sin', 'tan', 'arctan', 'random', 'randominit'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('-!+\\|=:;&<>()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
