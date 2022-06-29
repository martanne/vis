-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Bibtex LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('bibtex')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)

-- Fields.
lex:add_rule('field', token('field', word_match{
  'author', 'title', 'journal', 'year', 'volume', 'number', 'pages', 'month', 'note', 'key',
  'publisher', 'editor', 'series', 'address', 'edition', 'howpublished', 'booktitle',
  'organization', 'chapter', 'school', 'institution', 'type', 'isbn', 'issn', 'affiliation',
  'issue', 'keyword', 'url'
}))
lex:add_style('field', lexer.styles.constant)

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local dq_str = lexer.range('"')
local br_str = lexer.range('{', '}', false, false, true)
lex:add_rule('string', token(lexer.STRING, dq_str + br_str))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S(',=')))

-- Embedded in Latex.
local latex = lexer.load('latex')

-- Embedded Bibtex.
local entry = token('entry', '@' * word_match({
  'book', 'article', 'booklet', 'conference', 'inbook', 'incollection', 'inproceedings', 'manual',
  'mastersthesis', 'lambda', 'misc', 'phdthesis', 'proceedings', 'techreport', 'unpublished'
}, true))
lex:add_style('entry', lexer.styles.preprocessor)
local bibtex_start_rule = entry * ws^0 * token(lexer.OPERATOR, '{')
local bibtex_end_rule = token(lexer.OPERATOR, '}')
latex:embed(lex, bibtex_start_rule, bibtex_end_rule)

return lex
