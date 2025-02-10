-- Copyright 2023-2025 Mitchell. See LICENSE.
-- Objeck LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Class variables.
lex:add_rule('variable', lex:tag(lexer.VARIABLE, '@' * lexer.word))

-- Comments.
local line_comment = lexer.to_eol('#', true)
local block_comment = lexer.range('#~', '~#')
lex:add_rule('comment', lex:tag(lexer.COMMENT, block_comment + line_comment))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local ml_str = lexer.range('"', false, false)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + ml_str + dq_str))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('~!.,:;+-*/<>=\\^|&%?()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '#~', '~#')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'class', 'method', 'function', 'public', 'abstract', 'private', 'static', 'native', 'virtual',
	'Parent', 'As', 'from', 'implements', 'interface', 'enum', 'alias', 'consts', 'bundle', 'use',
	'leaving', 'if', 'else', 'do', 'while', 'select', 'break', 'continue', 'other', 'for', 'each',
	'reverse', 'label', 'return', 'critical', 'New', 'and', 'or', 'xor', 'true', 'false' -- , 'Nil'
})

lex:set_word_list(lexer.TYPE, {
	'Nil', 'Byte', 'ByteHolder', 'Int', 'IntHolder', 'Float', 'FloatHolder', 'Char', 'CharHolder',
	'Bool', 'BoolHolder', 'String', 'BaseArrayHolder', 'BoolArrayHolder', 'ByteArrayHolder',
	'CharArrayHolder', 'FloatArrayHolder', 'IntArrayHolder', 'StringArrayHolder', 'Func2Holder',
	'Func3Holder', 'Func4Holder', 'FuncHolder'
})

lexer.property['scintillua.comment'] = '#'

return lex
