-- Copyright 2021-2025 Mitchell. See LICENSE.
-- Hare LPeg lexer

local lexer = lexer
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Functions.
local builtin_func = lex:tag(lexer.FUNCTION_BUILTIN,
	lex:word_match(lexer.FUNCTION_BUILTIN) + 'size' * #(lexer.space^0 * '('))
local func = lex:tag(lexer.FUNCTION, lex:tag(lexer.FUNCTION, lexer.word * ('::' * lexer.word)^0 *
	#(lexer.space^0 * '(')))
lex:add_rule('function', builtin_func + func)

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"')
local raw_str = lexer.range('`', false, false)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str + raw_str))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('//')))

-- Numbers.
local bin_num = '0b' * R('01')^1 * -lexer.xdigit
local oct_num = '0o' * R('07')^1 * -lexer.xdigit
local hex_num = '0x' * lexer.xdigit^1
local int_suffix = lexer.word_match('i u z i8 i16 i32 i64 u8 u16 u32 u64')
local float_suffix = lexer.word_match('f32 f64')
local suffix = int_suffix + float_suffix
local integer = S('+-')^-1 *
	((hex_num + oct_num + bin_num) * int_suffix^-1 + lexer.dec_num * suffix^-1)
local float = lexer.float * float_suffix^-1
lex:add_rule('number', lex:tag(lexer.NUMBER, integer + float))

-- Error assertions
lex:add_rule('error_assert', lex:tag(lexer.ERROR .. '.assert', lpeg.B(')') * P('!')))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%^!=&|?~:;,.()[]{}<>')))

-- Attributes.
lex:add_rule('attribute', lex:tag(lexer.ANNOTATION, '@' * lexer.word))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'as', 'break', 'case', 'const', 'continue', 'def', 'defer', 'else', 'export', 'fn', 'for', 'if',
	'is', 'let', 'match', 'nullable', 'return', 'static', 'switch', 'type', 'use', 'yield', '_'
})

lex:set_word_list(lexer.TYPE, {
	'bool', 'enum', 'f32', 'f64', 'i16', 'i32', 'i64', 'i8', 'int', 'opaque', 'never', 'rune', 'size',
	'str', 'struct', 'u16', 'u32', 'u64', 'u8', 'uint', 'uintptr', 'union', 'valist'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'abort', 'align', 'alloc', 'append', 'assert', 'delete', 'free', 'insert', 'len', 'offset',
	'vaarg', 'vaend', 'vastart'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, 'done false null true void')

lexer.property['scintillua.comment'] = '//'

return lex
