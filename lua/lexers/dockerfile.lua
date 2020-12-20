-- Copyright 2016-2020 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- Dockerfile LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('dockerfile', {fold_by_indentation = true})

-- Whitespace
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  ADD ARG CMD COPY ENTRYPOINT ENV EXPOSE FROM LABEL MAINTAINER ONBUILD RUN
  STOPSIGNAL USER VOLUME WORKDIR
]]))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Variable.
lex:add_rule('variable', token(lexer.VARIABLE, S('$')^1 *
  (S('{')^1 * lexer.word * S('}')^1 + lexer.word)))

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('\\[],=:{}')))

return lex
