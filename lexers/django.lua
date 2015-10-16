-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Django LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = {_NAME = 'django'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '{#' * (l.any - l.newline - '#}')^0 *
                                 P('#}')^-1)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', false, true))

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'as', 'block', 'blocktrans', 'by', 'endblock', 'endblocktrans', 'comment',
  'endcomment', 'cycle', 'date', 'debug', 'else', 'extends', 'filter',
  'endfilter', 'firstof', 'for', 'endfor', 'if', 'endif', 'ifchanged',
  'endifchanged', 'ifnotequal', 'endifnotequal', 'in', 'load', 'not', 'now',
  'or', 'parsed', 'regroup', 'ssi', 'trans', 'with', 'widthratio'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'add', 'addslashes', 'capfirst', 'center', 'cut', 'date', 'default',
  'dictsort', 'dictsortreversed', 'divisibleby', 'escape', 'filesizeformat',
  'first', 'fix_ampersands', 'floatformat', 'get_digit', 'join', 'length',
  'length_is', 'linebreaks', 'linebreaksbr', 'linenumbers', 'ljust', 'lower',
  'make_list', 'phone2numeric', 'pluralize', 'pprint', 'random', 'removetags',
  'rjust', 'slice', 'slugify', 'stringformat', 'striptags', 'time', 'timesince',
  'title', 'truncatewords', 'unordered_list', 'upper', 'urlencode', 'urlize',
  'urlizetrunc', 'wordcount', 'wordwrap', 'yesno',
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S(':,.|'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'operator', operator},
}

-- Embedded in HTML.
local html = l.load('html')

-- Embedded Django.
local django_start_rule = token('django_tag', '{' * S('{%'))
local django_end_rule = token('django_tag', S('%}') * '}')
l.embed_lexer(html, M, django_start_rule, django_end_rule)
-- Modify HTML patterns to embed Django.
html._RULES['comment'] = html._RULES['comment'] + comment

M._tokenstyles = {
  django_tag = l.STYLE_EMBEDDED
}

local _foldsymbols = html._foldsymbols
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '{[%%{]'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '[%%}]}'
_foldsymbols.django_tag = {['{{'] = 1, ['}}'] = -1, ['{%'] = 1, ['%}'] = -1}
M._foldsymbols = _foldsymbols

return M
