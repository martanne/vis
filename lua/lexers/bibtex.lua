-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Bibtex LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'bibtex'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Strings.
local string = token(l.STRING, l.delimited_range('"') +
                               l.delimited_range('{}', false, true, true))

-- Fields.
local field = token('field', word_match{
  'author', 'title', 'journal', 'year', 'volume', 'number', 'pages', 'month',
  'note', 'key', 'publisher', 'editor', 'series', 'address', 'edition',
  'howpublished', 'booktitle', 'organization', 'chapter', 'school',
  'institution', 'type', 'isbn', 'issn', 'affiliation', 'issue', 'keyword',
  'url'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S(',='))

M._rules = {
  {'whitespace', ws},
  {'field', field},
  {'identifier', identifier},
  {'string', string},
  {'operator', operator},
}

-- Embedded in Latex.
local latex = l.load('latex')

-- Embedded Bibtex.
local entry = token('entry', P('@') * word_match({
  'book', 'article', 'booklet', 'conference', 'inbook', 'incollection',
  'inproceedings', 'manual', 'mastersthesis', 'lambda', 'misc', 'phdthesis',
  'proceedings', 'techreport', 'unpublished'
}, nil, true))
local bibtex_start_rule = entry * ws^0 * token(l.OPERATOR, P('{'))
local bibtex_end_rule = token(l.OPERATOR, P('}'))
l.embed_lexer(latex, M, bibtex_start_rule, bibtex_end_rule)

M._tokenstyles = {
  field = l.STYLE_CONSTANT,
  entry = l.STYLE_PREPROCESSOR
}

return M
