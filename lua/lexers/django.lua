-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Django LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
lex:add_rule('function',
	lpeg.B('|') * lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', lex:tag(lexer.STRING, lexer.range('"', false, false)))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S(':,.|')))

-- Embed Django in HTML.
local html = lexer.load('html')
html:add_rule('django_comment', lex:tag(lexer.COMMENT, lexer.range('{#', '#}', true)))
local django_start_rule = lex:tag(lexer.PREPROCESSOR, '{' * S('{%'))
local django_end_rule = lex:tag(lexer.PREPROCESSOR, S('%}') * '}')
html:embed(lex, django_start_rule, django_end_rule)

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, '{{', '}}')
lex:add_fold_point(lexer.PREPROCESSOR, '{%', '%}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'autoescape', 'endautoescape', 'block', 'endblock', 'comment', 'endcomment', 'csrf_token',
	'cycle', 'as', 'debug', 'extends', 'filter', 'endfilter', 'firstof', 'for', 'in', 'endfor',
	'empty', 'if', 'elif', 'else', 'endif', 'and', 'or', 'not', 'is', 'ifchanged', 'endifchanged',
	'include', 'load', 'lorem', 'now', 'regroup', 'resetcycle', 'spaceless', 'endspaceless',
	'templatetag', 'url', 'verbatim', 'endverbatim', 'widthratio', 'with', 'endwith', --
	'blocktranslate', 'endblocktranslate', 'translate', 'language', 'get_available_languages',
	'get_current_language', 'get_current_language_bidi', 'get_language_info',
	'get_language_info_list', --
	'get_static_prefix', 'get_media_prefix'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'add', 'addslashes', 'capfirst', 'center', 'cut', 'date', 'default', 'default_if_none',
	'dictsort', 'dictsortreversed', 'divisibleby', 'escape', 'escapejs', 'filesizeformat', 'first',
	'floatformat', 'force_escape', 'get_digit', 'iriencode', 'join', 'json_script', 'last', 'length',
	'length_is', 'linebreaks', 'linebreaksbr', 'linenumbers', 'ljust', 'lower', 'make_list',
	'phone2numeric', 'pluralize', 'pprint', 'random', 'rjust', 'safe', 'safeseq', 'slice', 'slugify',
	'stringformat', 'striptags', 'time', 'timesince', 'timeuntil', 'title', 'truncatechars_html',
	'truncatewords', 'truncatewords_html', 'unordered_list', 'upper', 'urlencode', 'urlize',
	'urlizetrunc', 'wordcount', 'wordwrap', 'yesno', --
	'language_name', 'language_name_local', 'language_bidi', 'language_name_translated'
})

lexer.property['scintillua.comment'] = '{#|#}'

return lex
