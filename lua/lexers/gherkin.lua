-- Copyright 2015-2022 Jason Schindler. See LICENSE.
-- Gherkin (https://github.com/cucumber/cucumber/wiki/Gherkin) LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('gherkin', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match(
  'And Background But Examples Feature Given Outline Scenario Scenarios Then When')))

-- Strings.
local doc_str = lexer.range('"""')
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, doc_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
-- lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Tags.
lex:add_rule('tag', token('tag', '@' * lexer.word^0))
lex:add_style('tag', lexer.styles.label)

-- Placeholders.
lex:add_rule('placeholder', token('placeholder', lexer.range('<', '>', false, false, true)))
lex:add_style('placeholder', lexer.styles.variable)

-- Examples.
lex:add_rule('example', token('example', lexer.to_eol('|')))
lex:add_style('example', lexer.styles.number)

return lex
