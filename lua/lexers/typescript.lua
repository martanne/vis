-- Copyright 2021-2022 Mitchell. See LICENSE.
-- TypeScript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('typescript', {inherit = lexer.load('javascript')})

-- Whitespace
lex:modify_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:modify_rule('keyword', token(lexer.KEYWORD, word_match[[
  abstract as constructor declare is module namespace require type
]]) + lex:get_rule('keyword'))

-- Types.
lex:modify_rule('type', token(lexer.TYPE, word_match[[
  boolean number bigint string unknown any void never symbol object
]]) + lex:get_rule('type'))

return lex
