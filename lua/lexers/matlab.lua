-- Copyright 2006-2017 Martin Morawetz. See LICENSE.
-- Matlab LPeg lexer.
-- Based off of lexer code by Mitchell.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'matlab'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = (P('%') + '#') * l.nonnewline^0
local block_comment = '%{' * (l.any - '%}')^0 * P('%}')^-1
local comment = token(l.COMMENT, block_comment + line_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"')
local bt_str = l.delimited_range('`')
local string = token(l.STRING, sq_str + dq_str + bt_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer + l.dec_num + l.hex_num +
                               l.oct_num)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'break', 'case', 'catch', 'continue', 'do', 'else', 'elseif', 'end',
  'end_try_catch', 'end_unwind_protect', 'endfor', 'endif', 'endswitch',
  'endwhile', 'for', 'function', 'endfunction', 'global', 'if', 'otherwise',
  'persistent', 'replot', 'return', 'static', 'switch', 'try', 'until',
  'unwind_protect', 'unwind_protect_cleanup', 'varargin', 'varargout', 'while'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, word_match{
  'abs', 'any', 'argv','atan2', 'axes', 'axis', 'ceil', 'cla', 'clear', 'clf',
  'columns', 'cos', 'delete', 'diff', 'disp', 'doc', 'double', 'drawnow', 'exp',
  'figure', 'find', 'fix', 'floor', 'fprintf', 'gca', 'gcf', 'get', 'grid',
  'help', 'hist', 'hold', 'isempty', 'isnull', 'length', 'load', 'log', 'log10',
  'loglog', 'max', 'mean', 'median', 'min', 'mod', 'ndims', 'numel', 'num2str',
  'ones', 'pause', 'plot', 'printf', 'quit', 'rand', 'randn', 'rectangle',
  'rem', 'repmat', 'reshape', 'round', 'rows', 'save', 'semilogx', 'semilogy',
  'set', 'sign', 'sin', 'size', 'sizeof', 'size_equal', 'sort', 'sprintf',
  'squeeze', 'sqrt', 'std', 'strcmp', 'subplot', 'sum', 'tan', 'tic', 'title',
  'toc', 'uicontrol', 'who', 'xlabel', 'ylabel', 'zeros'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'EDITOR', 'I', 'IMAGEPATH', 'INFO_FILE', 'J', 'LOADPATH', 'OCTAVE_VERSION',
  'PAGER', 'PS1', 'PS2', 'PS4', 'PWD'
})

-- Variable.
local variable = token(l.VARIABLE, word_match{
  'ans', 'automatic_replot', 'default_return_value', 'do_fortran_indexing',
  'define_all_return_values', 'empty_list_elements_ok', 'eps', 'false',
  'gnuplot_binary', 'ignore_function_time_stamp', 'implicit_str_to_num_ok',
  'Inf', 'inf', 'NaN', 'nan', 'ok_to_lose_imaginary_part',
  'output_max_field_width', 'output_precision', 'page_screen_output', 'pi',
  'prefer_column_vectors', 'prefer_zero_one_indexing', 'print_answer_id_name',
  'print_empty_dimensions', 'realmax', 'realmin', 'resize_on_range_error',
  'return_last_computed_value', 'save_precision', 'silent_functions',
  'split_long_rows', 'suppress_verbose_help_message', 'treat_neg_dim_as_zero',
  'true', 'warn_assign_as_truth_value', 'warn_comma_in_global_decl',
  'warn_divide_by_zero', 'warn_function_name_clash',
  'whitespace_in_literal_matrix'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('!%^&*()[]{}-=+/\\|:;.,?<>~`´'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'constant', constant},
  {'variable', variable},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '[%(%)%[%]]', '%%[{}]?', '#'},
  [l.KEYWORD] = {
    ['if'] = 1, ['for'] = 1, ['while'] = 1, switch = 1, ['end'] = -1
  },
  [l.OPERATOR] = {['('] = 1, [')'] = -1, ['['] = 1, [']'] = -1},
  [l.COMMENT] = {
    ['%{'] = 1, ['%}'] = -1, ['%'] = l.fold_line_comments('%'),
    ['#'] = l.fold_line_comments('#')
  }
}

return M
