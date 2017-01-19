-- Copyright 2015-2017 Jason Schindler. See LICENSE.
-- Gherkin (https://github.com/cucumber/cucumber/wiki/Gherkin) LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'gherkin'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Tags.
local tag = token('tag', '@' * l.word^0)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local doc_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local dq_str = l.delimited_range('"')

local string = token(l.STRING, doc_str + dq_str)

-- Placeholders.
local placeholder = token('placeholder', l.nested_pair('<', '>'))

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'Given', 'When', 'Then', 'And', 'But'
})

-- Identifiers.
local identifier = token(l.KEYWORD, P('Scenario Outline') + word_match{
  'Feature', 'Background', 'Scenario', 'Scenarios', 'Examples'
})

-- Examples.
local example = token('example', '|' * l.nonnewline^0)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'tag', tag},
  {'placeholder', placeholder},
  {'keyword', keyword},
  {'identifier', identifier},
  {'example', example},
  {'string', string},
  {'number', number}
}

M._tokenstyles = {
  tag = l.STYLE_LABEL,
  placeholder = l.STYLE_NUMBER,
  example = l.STYLE_NUMBER
}

M._FOLDBYINDENTATION = true

return M
