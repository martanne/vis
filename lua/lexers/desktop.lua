-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Desktop Entry LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keys.
lex:add_rule('key', lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN)))

-- Values.
lex:add_rule('value', lex:tag(lexer.CONSTANT_BUILTIN, lexer.word_match('true false')))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.alpha * (lexer.alnum + S('_-'))^0))

-- Group headers.
local bracketed = lexer.range('[', ']')
lex:add_rule('header', lexer.starts_line(lex:tag(lexer.HEADING, bracketed)))

-- Locales.
lex:add_rule('locale', lex:tag(lexer.TYPE, bracketed))

-- Strings.
lex:add_rule('string', lex:tag(lexer.STRING, lexer.range('"')))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Field codes.
lex:add_rule('code', lex:tag(lexer.CONSTANT_BUILTIN, '%' * S('fFuUdDnNickvm')))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('=')))

-- Word lists.
lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'Type', 'Version', 'Name', 'GenericName', 'NoDisplay', 'Comment', 'Icon', 'Hidden', 'OnlyShowIn',
	'NotShowIn', 'TryExec', 'Exec', 'Exec', 'Path', 'Terminal', 'MimeType', 'Categories',
	'StartupNotify', 'StartupWMClass', 'URL'
})

lexer.property['scintillua.comment'] = '#'

return lex
