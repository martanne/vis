-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Pike LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('pike')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'break', 'case', 'catch', 'continue', 'default', 'do', 'else', 'for', 'foreach', 'gauge', 'if',
  'lambda', 'return', 'sscanf', 'switch', 'while', 'import', 'inherit',
  -- Type modifiers.
  'constant', 'extern', 'final', 'inline', 'local', 'nomask', 'optional', 'private', 'protected',
  'public', 'static', 'variant'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match(
  'array class float function int mapping mixed multiset object program string void')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = P('#')^-1 * lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('lLdDfF')^-1))

-- Preprocessors.
lex:add_rule('preprocessor', token(lexer.PREPROCESSOR, lexer.to_eol(lexer.starts_line('#'))))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('<>=!+-/*%&|^~@`.,:;()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
