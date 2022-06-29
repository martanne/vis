-- Copyright 2006-2022 Mitchell. See LICENSE.
-- LPeg lexer for the Icon programming language.
-- http://www.cs.arizona.edu/icon
-- Contributed by Carl Sturtivant.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('icon')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'break', 'by', 'case', 'create', 'default', 'do', 'else', 'end', 'every', 'fail', 'global', 'if',
  'initial', 'invocable', 'link', 'local', 'next', 'not', 'of', 'procedure', 'record', 'repeat',
  'return', 'static', 'suspend', 'then', 'to', 'until', 'while'
}))

-- Icon Keywords: unique to Icon.
lex:add_rule('special_keyword', token('special_keyword', '&' * word_match{
  'allocated', 'ascii', 'clock', 'collections', 'cset', 'current', 'date', 'dateline', 'digits',
  'dump', 'e', 'error', 'errornumber', 'errortext', 'errorvalue', 'errout', 'fail', 'features',
  'file', 'host', 'input', 'lcase', 'letters', 'level', 'line', 'main', 'null', 'output', 'phi',
  'pi', 'pos', 'progname', 'random', 'regions', 'source', 'storage', 'subject', 'time', 'trace',
  'ucase', 'version'
}))
lex:add_style('special_keyword', lexer.styles.type)

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#', true)))

-- Numbers.
local radix_literal = P('-')^-1 * lexer.dec_num * S('rR') * lexer.alnum^1
lex:add_rule('number', token(lexer.NUMBER, radix_literal + lexer.number))

-- Preprocessor.
lex:add_rule('preproc', token(lexer.PREPROCESSOR, '$' *
  word_match('define else endif error ifdef ifndef include line undef')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>~!=^&|?~@:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, 'ifdef', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifndef', 'endif')
lex:add_fold_point(lexer.KEYWORD, 'procedure', 'end')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
