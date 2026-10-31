-- Copyright © 2026 Michael T. Richter. See LICENSE.
-- Lexer for the Cangjie programming language.

local lexer = require('lexer')
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
local keyword = lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD))
local contextual_keyword = lex:tag(lexer.KEYWORD .. '.contextual',
	lex:word_match(lexer.KEYWORD .. '.contextual'))
lex:add_rule('keyword', keyword + contextual_keyword)

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
local identifier = lexer.word_utf8
local raw_identifier = '`' * identifier * '`'
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, raw_identifier + identifier))

-- Strings.
local sq_str = lexer.range('"', true)
local dq_str = lexer.range("'", true)
local ml_str = lexer.range('"""') + lexer.range("'''")
local raw_str = P('#')^1 * (lexer.range("'") + lexer.range('"')) * P('#')^1
local rune = 'r' * (lexer.range("'") + lexer.range('"'))
lex:add_rule('string', lex:tag(lexer.STRING, ml_str + sq_str + dq_str + raw_str + rune))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
local suffix = lexer.word_match('i8 i16 i32 i64 u8 u16 u32 u64 f16 f32 f64')
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_') * suffix^-1))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '..' + S('+-*/%=<>!&|^~?.@:[](){}')^1))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	-- Control flow.
	'if', 'else', 'match', 'case', 'for', 'in', 'while', 'do', 'break', 'continue', 'return', 'throw',
	'try', 'catch', 'finally', 'defer', 'guard', --
	-- Declarations.
	'class', 'struct', 'enum', 'interface', 'extension', 'typealias', 'func', 'let', 'var', 'package',
	'import', 'const', 'mut', 'macro', 'operator', 'prop', --
	-- Modifiers & Visibility.
	'static', 'final', 'native', 'extern', 'inline', 'synchronized', 'unsafe', --
	-- Expressions & Misc.
	'this', 'super', 'where', 'main', 'from', 'foreign', 'quote', 'spawn', 'typeof',
	-- Type test/cast.
	'is', 'as', 'init', 'inout', 'This', 'type'
})

lex:set_word_list(lexer.KEYWORD .. '.contextual', {
	'abstract', 'open', 'override', 'private', 'protected', 'public', 'redef', 'get', 'set', 'sealed'
})

lex:set_word_list(lexer.TYPE, {
	-- Numeric types.
	'Int8', 'Int16', 'Int32', 'Int64', 'IntNative', 'UInt8', 'UInt16', 'UInt32', 'UInt64',
	'UIntNative', 'Float16', 'Float32', 'Float64', --
	-- Numeric aliases.
	'Byte', 'Int', 'UInt', --
	-- Core types.
	'Bool', 'Rune', 'String', --
	-- Core mutable/generic types.
	'Array', 'VArray', 'Tuple', 'Range', 'Function', --
	-- Option type.
	'Option'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'true', 'false', 'null', 'Unit', 'Nothing', 'Some', 'None'
})

lexer.property['scintillua.comment'] = '//'

return lex
