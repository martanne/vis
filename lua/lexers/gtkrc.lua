-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Gtkrc LPeg lexer.

local lexer = lexer
local word_match = lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, word_match(
	'binding class include module_path pixmap_path im_module_file style widget widget_class')))

-- Variables.
lex:add_rule('variable', lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN)))

-- States.
lex:add_rule('state', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Functions.
lex:add_rule('function', lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.alpha * (lexer.alnum + S('_-'))^0))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.digit^1 * ('.' * lexer.digit^1)^-1))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S(':=,*()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'bg', 'fg', 'base', 'text', 'xthickness', 'ythickness', 'bg_pixmap', 'font', 'fontset',
	'font_name', 'stock', 'color', 'engine'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'ACTIVE', 'SELECTED', 'NORMAL', 'PRELIGHT', 'INSENSITIVE', 'TRUE', 'FALSE'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {'mix', 'shade', 'lighter', 'darker'})

lexer.property['scintillua.comment'] = '#'

return lex
