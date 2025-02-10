-- Copyright 2006-2025 Martin Morawetz. See LICENSE.
-- Matlab LPeg lexer.
-- Based off of lexer code by Mitchell.

local lexer = lexer
local P, B, S = lpeg.P, lpeg.B, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
-- Note: cannot tag normal functions because indexing variables uses the same syntax.
local builtin_func = lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
lex:add_rule('function', builtin_func * #(lexer.space^0 * S('(')))

-- Variable.
lex:add_rule('variable', lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str + bq_str))

-- Comments.
local line_comment = lexer.to_eol(S('%#'))
local block_comment = lexer.range('%{', '%}')
lex:add_rule('comment', lex:tag(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('!%^&*()[]{}-=+/\\|:;.,?<>~`´')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
lex:add_fold_point(lexer.KEYWORD, 'for', 'end')
lex:add_fold_point(lexer.KEYWORD, 'while', 'end')
lex:add_fold_point(lexer.KEYWORD, 'switch', 'end')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.COMMENT, '%{', '%}')

-- Word lists
lex:set_word_list(lexer.KEYWORD, {
	'break', 'case', 'catch', 'continue', 'do', 'else', 'elseif', 'end', 'end_try_catch',
	'end_unwind_protect', 'endfor', 'endif', 'endswitch', 'endwhile', 'for', 'function',
	'endfunction', 'global', 'if', 'otherwise', 'persistent', 'replot', 'return', 'static', 'switch',
	'try', 'until', 'unwind_protect', 'unwind_protect_cleanup', 'varargin', 'varargout', 'while'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'abs', 'any', 'argvatan2', 'axes', 'axis', 'ceil', 'cla', 'clear', 'clf', 'columns', 'cos',
	'delete', 'diff', 'disp', 'doc', 'double', 'drawnow', 'exp', 'figure', 'find', 'fix', 'floor',
	'fprintf', 'gca', 'gcf', 'get', 'grid', 'help', 'hist', 'hold', 'isempty', 'isnull', 'length',
	'load', 'log', 'log10', 'loglog', 'max', 'mean', 'median', 'min', 'mod', 'ndims', 'numel',
	'num2str', 'ones', 'pause', 'plot', 'printf', 'quit', 'rand', 'randn', 'rectangle', 'rem',
	'repmat', 'reshape', 'round', 'rows', 'save', 'semilogx', 'semilogy', 'set', 'sign', 'sin',
	'size', 'sizeof', 'size_equal', 'sort', 'sprintf', 'squeeze', 'sqrt', 'std', 'strcmp', 'subplot',
	'sum', 'tan', 'tic', 'title', 'toc', 'uicontrol', 'who', 'xlabel', 'ylabel', 'zeros'
})

lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'ans', 'automatic_replot', 'default_return_value', 'do_fortran_indexing',
	'define_all_return_values', 'empty_list_elements_ok', 'eps', 'gnuplot_binary',
	'ignore_function_time_stamp', 'implicit_str_to_num_ok', 'ok_to_lose_imaginary_part',
	'output_max_field_width', 'output_precision', 'page_screen_output', 'prefer_column_vectors',
	'prefer_zero_one_indexing', 'print_answer_id_name', 'print_empty_dimensions',
	'resize_on_range_error', 'return_last_computed_value', 'save_precision', 'silent_functions',
	'split_long_rows', 'suppress_verbose_help_message', 'treat_neg_dim_as_zero',
	'warn_assign_as_truth_value', 'warn_comma_in_global_decl', 'warn_divide_by_zero',
	'warn_function_name_clash', 'whitespace_in_literal_matrix'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'false', 'Inf', 'inf', 'NaN', 'nan', 'pi', 'realmax', 'realmin', 'true'
})

lexer.property['scintillua.comment'] = '%'

return lex
