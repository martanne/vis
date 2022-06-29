-- Copyright 2006-2022 Martin Morawetz. See LICENSE.
-- Matlab LPeg lexer.
-- Based off of lexer code by Mitchell.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('matlab')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'break', 'case', 'catch', 'continue', 'do', 'else', 'elseif', 'end', 'end_try_catch',
  'end_unwind_protect', 'endfor', 'endif', 'endswitch', 'endwhile', 'for', 'function',
  'endfunction', 'global', 'if', 'otherwise', 'persistent', 'replot', 'return', 'static', 'switch',
  'try', 'until', 'unwind_protect', 'unwind_protect_cleanup', 'varargin', 'varargout', 'while'
}, true)))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'abs', 'any', 'argvatan2', 'axes', 'axis', 'ceil', 'cla', 'clear', 'clf', 'columns', 'cos',
  'delete', 'diff', 'disp', 'doc', 'double', 'drawnow', 'exp', 'figure', 'find', 'fix', 'floor',
  'fprintf', 'gca', 'gcf', 'get', 'grid', 'help', 'hist', 'hold', 'isempty', 'isnull', 'length',
  'load', 'log', 'log10', 'loglog', 'max', 'mean', 'median', 'min', 'mod', 'ndims', 'numel',
  'num2str', 'ones', 'pause', 'plot', 'printf', 'quit', 'rand', 'randn', 'rectangle', 'rem',
  'repmat', 'reshape', 'round', 'rows', 'save', 'semilogx', 'semilogy', 'set', 'sign', 'sin',
  'size', 'sizeof', 'size_equal', 'sort', 'sprintf', 'squeeze', 'sqrt', 'std', 'strcmp', 'subplot',
  'sum', 'tan', 'tic', 'title', 'toc', 'uicontrol', 'who', 'xlabel', 'ylabel', 'zeros'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match(
  'EDITOR I IMAGEPATH INFO_FILE J LOADPATH OCTAVE_VERSION PAGER PS1 PS2 PS4 PWD')))

-- Variable.
lex:add_rule('variable', token(lexer.VARIABLE, word_match{
  'ans', 'automatic_replot', 'default_return_value', 'do_fortran_indexing',
  'define_all_return_values', 'empty_list_elements_ok', 'eps', 'false', 'gnuplot_binary',
  'ignore_function_time_stamp', 'implicit_str_to_num_ok', 'Inf', 'inf', 'NaN', 'nan',
  'ok_to_lose_imaginary_part', 'output_max_field_width', 'output_precision', 'page_screen_output',
  'pi', 'prefer_column_vectors', 'prefer_zero_one_indexing', 'print_answer_id_name',
  'print_empty_dimensions', 'realmax', 'realmin', 'resize_on_range_error',
  'return_last_computed_value', 'save_precision', 'silent_functions', 'split_long_rows',
  'suppress_verbose_help_message', 'treat_neg_dim_as_zero', 'true', 'warn_assign_as_truth_value',
  'warn_comma_in_global_decl', 'warn_divide_by_zero', 'warn_function_name_clash',
  'whitespace_in_literal_matrix'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + bq_str))

-- Comments.
local line_comment = lexer.to_eol(S('%#'))
local block_comment = lexer.range('%{', '%}')
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!%^&*()[]{}-=+/\\|:;.,?<>~`´')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
lex:add_fold_point(lexer.KEYWORD, 'for', 'end')
lex:add_fold_point(lexer.KEYWORD, 'while', 'end')
lex:add_fold_point(lexer.KEYWORD, 'switch', 'end')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.COMMENT, '%{', '%}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('%'))
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
