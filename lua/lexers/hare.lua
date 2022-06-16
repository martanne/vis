-- Copyright 2021-2022 Mitchell. See LICENSE.
-- Hare LPeg lexer
-- https://harelang.org
-- Contributed by Qiu

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('hare')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'as', 'break', 'case', 'const', 'continue', 'def', 'defer', 'else', 'export', 'false', 'fn',
  'for', 'if', 'is', 'let', 'match', 'null', 'nullable', 'return', 'static', 'struct', 'switch',
  'true', 'type', 'use', 'yield'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'len', 'alloc', 'free', 'assert', 'abort', 'size', 'append', 'insert', 'delete', 'vastart',
  'vaarg', 'vaend'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bool', 'enum', 'f32', 'f64', 'i16', 'i32', 'i64', 'i8', 'int', 'u16', 'u32', 'u64', 'u8', 'uint',
  'uintptr', 'union', 'void', 'rune', 'str', 'char'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local dq_str = lexer.range('"')
local raw_str = lexer.range('`')
lex:add_rule('string', token(lexer.STRING, dq_str + raw_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('//')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%^!=&|?:;,.()[]{}<>')))

-- At rule.
lex:add_rule('at_rule', token('at_rule', '@' * word_match('noreturn offset init fini test symbol')))
lex:add_style('at_rule', lexer.styles.preprocessor)

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
