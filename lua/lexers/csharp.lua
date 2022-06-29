-- Copyright 2006-2022 Mitchell. See LICENSE.
-- C# LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('csharp')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'class', 'delegate', 'enum', 'event', 'interface', 'namespace', 'struct', 'using', 'abstract',
  'const', 'explicit', 'extern', 'fixed', 'implicit', 'internal', 'lock', 'out', 'override',
  'params', 'partial', 'private', 'protected', 'public', 'ref', 'sealed', 'static', 'readonly',
  'unsafe', 'virtual', 'volatile', 'add', 'as', 'assembly', 'base', 'break', 'case', 'catch',
  'checked', 'continue', 'default', 'do', 'else', 'finally', 'for', 'foreach', 'get', 'goto', 'if',
  'in', 'is', 'new', 'remove', 'return', 'set', 'sizeof', 'stackalloc', 'super', 'switch', 'this',
  'throw', 'try', 'typeof', 'unchecked', 'value', 'var', 'void', 'while', 'yield', 'null', 'true',
  'false'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bool', 'byte', 'char', 'decimal', 'double', 'float', 'int', 'long', 'object', 'operator',
  'sbyte', 'short', 'string', 'uint', 'ulong', 'ushort'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local ml_str = P('@')^-1 * lexer.range('"', false, false)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + ml_str))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('lLdDfFmM')^-1))

-- Preprocessor.
lex:add_rule('preprocessor', token(lexer.PREPROCESSOR, '#' * S('\t ')^0 *
  word_match('define elif else endif error if line undef warning region endregion')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('~!.,:;+-*/<>=\\^|&%?()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, 'if', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifdef', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifndef', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'region', 'endregion')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
