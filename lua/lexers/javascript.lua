-- Copyright 2006-2020 Mitchell. See LICENSE.
-- JavaScript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('javascript')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  abstract async await boolean break byte case catch char class const continue
  debugger default delete do double else enum export extends false final finally
  float for function get goto if implements import in instanceof int interface
  let long native new null of package private protected public return set short
  static super switch synchronized this throw throws transient true try typeof
  var void volatile while with yield
]]))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
local string = token(lexer.STRING, sq_str + dq_str + bq_str)
local regex_str = #P('/') * lexer.last_char_includes('+-*%^!=&|?:;,([{<>') *
  lexer.range('/', true) * S('igm')^0
local regex = token(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%^!=&|?:;,.()[]{}<>')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
