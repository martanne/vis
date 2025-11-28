-- Copyright 2015-2025 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- Rust LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S
local C, Cmt = lpeg.C, lpeg.Cmt

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Library types.
lex:add_rule('library', lex:tag(lexer.TYPE, lexer.upper * (lexer.lower + lexer.dec_num)^1))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Lifetime annotation.
lex:add_rule('lifetime', lex:tag(lexer.OPERATOR, S('<&') * P("'")))

-- Strings.
local sq_str = P('b')^-1 * lexer.range("'", true)
local dq_str = P('b')^-1 * lexer.range('"')
local raw_str = Cmt(P('b')^-1 * P('r') * C(P('#')^0) * '"', function(input, index, hashes)
	local _, e = input:find('"' .. hashes, index, true)
	return (e or #input) + 1
end)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str + raw_str))

-- Functions.
local builtin_macros = lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN) * '!')
local macros = lex:tag(lexer.FUNCTION, lexer.word * '!')
local func = lex:tag(lexer.FUNCTION, lexer.word)
lex:add_rule('function', (builtin_macros + macros + func) * #(lexer.space^0 * '('))

-- Identifiers.
local identifier = P('r#')^-1 * lexer.word
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, identifier))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_')))

-- Attributes.
lex:add_rule('preprocessor', lex:tag(lexer.PREPROCESSOR, '#' * lexer.range('[', ']', true)))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '..' + S('+-/*%<>!=`^~@&|?#~:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- https://doc.rust-lang.org/std/#keywords
lex:set_word_list(lexer.KEYWORD, {
	'SelfTy', 'as', 'async', 'await', 'break', 'const', 'continue', 'crate', 'dyn', 'else', 'enum',
	'extern', 'false', 'fn', 'for', 'if', 'impl', 'in', 'let', 'loop', 'match', 'mod', 'move', 'mut',
	'pub', 'ref', 'return', 'self', 'static', 'struct', 'super', 'trait', 'true', 'type', 'union',
	'unsafe', 'use', 'where', 'while'
})

-- https://doc.rust-lang.org/std/#primitives
lex:set_word_list(lexer.TYPE, {
	'never', 'array', 'bool', 'char', 'f32', 'f64', 'fn', 'i8', 'i16', 'i32', 'i64', 'i128', 'isize',
	'pointer', 'reference', 'slice', 'str', 'tuple', 'u8', 'u16', 'u32', 'u64', 'u128', 'unit',
	'usize'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'assert', 'assert_eq', 'assert_ne', 'cfg', 'column', 'compile_error', 'concat', 'dbg',
	'debug_assert', 'debug_assert_eq', 'debug_assert_ne', 'env', 'eprint', 'eprintln', 'file',
	'format', 'format_args', 'include', 'include_bytes', 'include_str', 'line', 'matches',
	'module_path', 'option_env', 'panic', 'print', 'println', 'stringify', 'thread_local', 'todo',
	'unimplemented', 'unreachable', 'vec', 'write', 'writeln',
	-- Experimental
	'concat_bytes', 'concat_idents', 'const_format_args', 'format_args_nl', 'log_syntax',
	'trace_macros',
	-- Deprecated
	'try'
})

lexer.property['scintillua.comment'] = '//'

return lex
