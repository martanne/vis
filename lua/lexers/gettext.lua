-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Gettext LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('gettext')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match(
  'msgid msgid_plural msgstr fuzzy c-format no-c-format', true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, S('%$@') * lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true)))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#' * S(': .~'))))

return lex
