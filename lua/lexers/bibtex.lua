-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Bibtex LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Fields.
lex:add_rule('field', lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN, true)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local dq_str = lexer.range('"')
local br_str = lexer.range('{', '}', false, false, true)
lex:add_rule('string', lex:tag(lexer.STRING, dq_str + br_str))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S(',=')))

-- Embedded in Latex.
local latex = lexer.load('latex')

-- Embedded Bibtex.
local entry = lex:tag(lexer.PREPROCESSOR, '@' * lex:word_match('entry', true))
local bibtex_start_rule = entry * lex:get_rule('whitespace')^0 * lex:tag(lexer.OPERATOR, '{')
local bibtex_end_rule = lex:tag(lexer.OPERATOR, '}')
latex:embed(lex, bibtex_start_rule, bibtex_end_rule)

-- Word lists.
lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'author', 'title', 'journal', 'year', 'volume', 'number', 'pages', 'month', 'note', 'key',
	'publisher', 'editor', 'series', 'address', 'edition', 'howpublished', 'booktitle',
	'organization', 'chapter', 'school', 'institution', 'type', 'isbn', 'issn', 'affiliation',
	'issue', 'keyword', 'url'
})

lex:set_word_list('entry', {
	'string', --
	'book', 'article', 'booklet', 'conference', 'inbook', 'incollection', 'inproceedings', 'manual',
	'mastersthesis', 'lambda', 'misc', 'phdthesis', 'proceedings', 'techreport', 'unpublished'
})

lexer.property['scintillua.comment'] = '%'

return lex
