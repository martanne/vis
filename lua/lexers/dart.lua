-- Copyright 2013-2025 Mitchell. See LICENSE.
-- Dart LPeg lexer.
-- Written by Brian Schott (@Hackerpilot on Github).
-- Migrated by Jamie Drinkell

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))
-- Built-ins.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))
-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))
-- Directives
lex:add_rule('directive', lex:tag(lexer.PREPROCESSOR, lex:word_match(lexer.PREPROCESSOR)))

-- Strings.
local sq_str = S('r')^-1 * lexer.range("'", true)
local dq_str = S('r')^-1 * lexer.range('"', true)
local tq_str = S('r')^-1 * (lexer.range("'''") + lexer.range('"""'))
lex:add_rule('string', lex:tag(lexer.STRING, tq_str + sq_str + dq_str))

-- Functions.
lex:add_rule('function', lex:tag(lexer.FUNCTION, lexer.word) * #(lexer.space^0 * '('))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('#?=!<>+-*$/%&|^~.,;()[]{}')))

-- Annotations.
lex:add_rule('annotation', lex:tag(lexer.ANNOTATION, '@' * lexer.word^1))

-- Fold points (add for most bracket pairs due to Flutter's usual formatting).
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

lex:set_word_list(lexer.KEYWORD, {
	'abstract', 'as', 'assert', 'async', 'await', 'break', 'case', 'catch', 'class', 'continue',
	'covariant', 'default', 'do', 'else', 'enum', 'extends', 'factory', 'finally', 'for', 'get', 'if',
	'implements', 'in', 'interface', 'is', 'mixin', 'on', 'operator', 'rethrow', 'return', 'set',
	'super', 'switch', 'sync', 'this', 'throw', 'try', 'with', 'while', 'yield', --
	'base', 'extension', 'external', 'late', 'of', 'required', 'sealed', 'when'
})

lex:set_word_list(lexer.PREPROCESSOR, {
	'deferred', 'export', 'hide', 'import', 'library', 'part'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'false', 'true', 'null'
})

lex:set_word_list(lexer.TYPE, {
	'const', 'dynamic', 'final', 'Function', 'new', 'static', 'typedef', 'var', 'void', 'int',
	'double', 'String', 'bool', 'List', 'Set', 'Map', 'Future', 'Stream', 'Iterable', 'Object',
	'Null', 'type'
})

lexer.property['scintillua.comment'] = '//'

return lex
