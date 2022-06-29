-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Boo LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('boo')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'and', 'break', 'cast', 'continue', 'elif', 'else', 'ensure', 'except', 'for', 'given', 'goto',
  'if', 'in', 'isa', 'is', 'not', 'or', 'otherwise', 'pass', 'raise', 'ref', 'try', 'unless',
  'when', 'while',
  -- Definitions.
  'abstract', 'callable', 'class', 'constructor', 'def', 'destructor', 'do', 'enum', 'event',
  'final', 'get', 'interface', 'internal', 'of', 'override', 'partial', 'private', 'protected',
  'public', 'return', 'set', 'static', 'struct', 'transient', 'virtual', 'yield',
  -- Namespaces.
  'as', 'from', 'import', 'namespace',
  -- Other.
  'self', 'super', 'null', 'true', 'false'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bool', 'byte', 'char', 'date', 'decimal', 'double', 'duck', 'float', 'int', 'long', 'object',
  'operator', 'regex', 'sbyte', 'short', 'single', 'string', 'timespan', 'uint', 'ulong', 'ushort'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'array', 'assert', 'checked', 'enumerate', '__eval__', 'filter', 'getter', 'len', 'lock', 'map',
  'matrix', 'max', 'min', 'normalArrayIndexing', 'print', 'property', 'range', 'rawArrayIndexing',
  'required', '__switch__', 'typeof', 'unchecked', 'using', 'yieldAll', 'zip'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local tq_str = lexer.range('"""')
local string = token(lexer.STRING, tq_str + sq_str + dq_str)
local regex_str = #P('/') * lexer.last_char_includes('!%^&*([{-=+|:;,?<>~') * lexer.range('/', true)
local regex = token(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Comments.
local line_comment = lexer.to_eol('#', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * (S('msdhsfFlL') + 'ms')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!%^&*()[]{}-=+/|:;.,?<>~`')))

return lex
