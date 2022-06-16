-- Copyright 2018-2022 Hugo O. Rivera. See LICENSE.
-- Reason (https://reasonml.github.io/) LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('reason')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'and', 'as', 'asr', 'begin', 'class', 'closed', 'constraint', 'do', 'done', 'downto', 'else',
  'end', 'exception', 'external', 'failwith', 'false', 'flush', 'for', 'fun', 'function', 'functor',
  'if', 'in', 'include', 'inherit', 'incr', 'land', 'let', 'load', 'los', 'lsl', 'lsr', 'lxor',
  'method', 'mod', 'module', 'mutable', 'new', 'not', 'of', 'open', 'option', 'or', 'parser',
  'private', 'ref', 'rec', 'raise', 'regexp', 'sig', 'struct', 'stdout', 'stdin', 'stderr',
  'switch', 'then', 'to', 'true', 'try', 'type', 'val', 'virtual', 'when', 'while', 'with'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match('int float bool char string unit')))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'raise', 'invalid_arg', 'failwith', 'compare', 'min', 'max', 'succ', 'pred', 'mod', 'abs',
  'max_int', 'min_int', 'sqrt', 'exp', 'log', 'log10', 'cos', 'sin', 'tan', 'acos', 'asin', 'atan',
  'atan2', 'cosh', 'sinh', 'tanh', 'ceil', 'floor', 'abs_float', 'mod_float', 'frexp', 'ldexp',
  'modf', 'float', 'float_of_int', 'truncate', 'int_of_float', 'infinity', 'nan', 'max_float',
  'min_float', 'epsilon_float', 'classify_float', 'int_of_char', 'char_of_int', 'ignore',
  'string_of_bool', 'bool_of_string', 'string_of_int', 'int_of_string', 'string_of_float',
  'float_of_string', 'fst', 'snd', 'stdin', 'stdout', 'stderr', 'print_char', 'print_string',
  'print_int', 'print_float', 'print_endline', 'print_newline', 'prerr_char', 'prerr_string',
  'prerr_int', 'prerr_float', 'prerr_endline', 'prerr_newline', 'read_line', 'read_int',
  'read_float', 'open_out', 'open_out_bin', 'open_out_gen', 'flush', 'flush_all', 'output_char',
  'output_string', 'output', 'output_byte', 'output_binary_int', 'output_value', 'seek_out',
  'pos_out', 'out_channel_length', 'close_out', 'close_out_noerr', 'set_binary_mode_out', 'open_in',
  'open_in_bin', 'open_in_gen', 'input_char', 'input_line', 'input', 'really_input', 'input_byte',
  'input_binary_int', 'input_value', 'seek_in', 'pos_in', 'in_channel_length', 'close_in',
  'close_in_noerr', 'set_binary_mode_in', 'incr', 'decr', 'string_of_format', 'format_of_string',
  'exit', 'at_exit'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=<>+-*/.,:;~!#%^&|?[](){}')))

return lex
