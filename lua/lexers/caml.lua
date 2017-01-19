-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- OCaml LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'caml'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, l.nested_pair('(*', '*)'))

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'and', 'as', 'asr', 'begin', 'class', 'closed', 'constraint', 'do', 'done',
  'downto', 'else', 'end', 'exception', 'external', 'failwith', 'false',
  'flush', 'for', 'fun', 'function', 'functor', 'if', 'in', 'include',
  'inherit',  'incr', 'land', 'let', 'load', 'los', 'lsl', 'lsr', 'lxor',
  'match', 'method', 'mod', 'module', 'mutable', 'new', 'not', 'of', 'open',
  'option', 'or', 'parser', 'private', 'ref', 'rec', 'raise', 'regexp', 'sig',
  'struct', 'stdout', 'stdin', 'stderr', 'then', 'to', 'true', 'try', 'type',
  'val', 'virtual', 'when', 'while', 'with'
})

-- Types.
local type = token(l.TYPE, word_match{
  'int', 'float', 'bool', 'char', 'string', 'unit'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'raise', 'invalid_arg', 'failwith', 'compare', 'min', 'max', 'succ', 'pred',
  'mod', 'abs', 'max_int', 'min_int', 'sqrt', 'exp', 'log', 'log10', 'cos',
  'sin', 'tan', 'acos', 'asin', 'atan', 'atan2', 'cosh', 'sinh', 'tanh', 'ceil',
  'floor', 'abs_float', 'mod_float', 'frexp', 'ldexp', 'modf', 'float',
  'float_of_int', 'truncate', 'int_of_float', 'infinity', 'nan', 'max_float',
  'min_float', 'epsilon_float', 'classify_float', 'int_of_char', 'char_of_int',
  'ignore', 'string_of_bool', 'bool_of_string', 'string_of_int',
  'int_of_string', 'string_of_float', 'float_of_string', 'fst', 'snd', 'stdin',
  'stdout', 'stderr', 'print_char', 'print_string', 'print_int', 'print_float',
  'print_endline', 'print_newline', 'prerr_char', 'prerr_string', 'prerr_int',
  'prerr_float', 'prerr_endline', 'prerr_newline', 'read_line', 'read_int',
  'read_float', 'open_out', 'open_out_bin', 'open_out_gen', 'flush',
  'flush_all', 'output_char', 'output_string', 'output', 'output_byte',
  'output_binary_int', 'output_value', 'seek_out', 'pos_out',
  'out_channel_length', 'close_out', 'close_out_noerr', 'set_binary_mode_out',
  'open_in', 'open_in_bin', 'open_in_gen', 'input_char', 'input_line', 'input',
  'really_input', 'input_byte', 'input_binary_int', 'input_value', 'seek_in',
  'pos_in', 'in_channel_length', 'close_in', 'close_in_noerr',
  'set_binary_mode_in', 'incr', 'decr', 'string_of_format', 'format_of_string',
  'exit', 'at_exit'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('=<>+-*/.,:;~!#%^&|?[](){}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
