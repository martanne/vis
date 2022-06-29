-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Groovy LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('groovy')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abstract', 'break', 'case', 'catch', 'continue', 'default', 'do', 'else', 'extends', 'final',
  'finally', 'for', 'if', 'implements', 'instanceof', 'native', 'new', 'private', 'protected',
  'public', 'return', 'static', 'switch', 'synchronized', 'throw', 'throws', 'transient', 'try',
  'volatile', 'while', 'strictfp', 'package', 'import', 'as', 'assert', 'def', 'mixin', 'property',
  'test', 'using', 'in', 'false', 'null', 'super', 'this', 'true', 'it'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'abs', 'any', 'append', 'asList', 'asWritable', 'call', 'collect', 'compareTo', 'count', 'div',
  'dump', 'each', 'eachByte', 'eachFile', 'eachLine', 'every', 'find', 'findAll', 'flatten',
  'getAt', 'getErr', 'getIn', 'getOut', 'getText', 'grep', 'immutable', 'inject', 'inspect',
  'intersect', 'invokeMethods', 'isCase', 'join', 'leftShift', 'minus', 'multiply',
  'newInputStream', 'newOutputStream', 'newPrintWriter', 'newReader', 'newWriter', 'next', 'plus',
  'pop', 'power', 'previous', 'print', 'println', 'push', 'putAt', 'read', 'readBytes', 'readLines',
  'reverse', 'reverseEach', 'round', 'size', 'sort', 'splitEachLine', 'step', 'subMap', 'times',
  'toInteger', 'toList', 'tokenize', 'upto', 'waitForOrKill', 'withPrintWriter', 'withReader',
  'withStream', 'withWriter', 'withWriterAppend', 'write', 'writeLine'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match(
  'boolean byte char class double float int interface long short void')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local tq_str = lexer.range("'''") + lexer.range('"""')
local string = token(lexer.STRING, tq_str + sq_str + dq_str)
local regex_str = #P('/') * lexer.last_char_includes('=~|!<>+-*?&,:;([{') * lexer.range('/', true)
local regex = token(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=~|!<>+-/*?&.,:;()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
