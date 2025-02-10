-- Copyright 2006-2025 Mitchell. See LICENSE.
-- MediaWiki LPeg lexer.
-- Contributed by Alexander Misel.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new('mediawiki')

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.range('<!--', '-->')))

-- HTML-like tags
local tag_start = token(lexer.TAG, '<' * P('/')^-1 * lexer.alnum^1 * lexer.space^0)
local dq_str = '"' * ((lexer.any - S('>"\\')) + ('\\' * lexer.any))^0 * '"'
local tag_attr = token(lexer.ATTRIBUTE, lexer.alpha^1 * lexer.space^0 *
	('=' * lexer.space^0 * (dq_str + (lexer.any - lexer.space - '>')^0)^-1)^0 * lexer.space^0)
local tag_end = token(lexer.TAG, P('/')^-1 * '>')
lex:add_rule('tag', tag_start * tag_attr^0 * tag_end)

-- Link
lex:add_rule('link', token(lexer.STRING, S('[]')))
lex:add_rule('internal_link', B('[[') * token(lexer.LINK, (lexer.any - '|' - ']]')^1))

-- Templates and parser functions.
lex:add_rule('template', token(lexer.OPERATOR, S('{}')))
lex:add_rule('parser_func',
	B('{{') * token(lexer.FUNCTION, '#' * lexer.alpha^1 + lexer.upper^1 * ':'))
lex:add_rule('template_name', B('{{') * token(lexer.LINK, (lexer.any - S('{}|'))^1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('-=|#~!')))

-- Behavior switches
local start_pat = P(function(_, pos) return pos == 1 end)
lex:add_rule('behavior_switch', (B(lexer.space) + start_pat) * token('behavior_switch', word_match(
	'__TOC__ __FORCETOC__ __NOTOC__ __NOEDITSECTION__ __NOCC__ __NOINDEX__')) * #lexer.space)
lex:add_style('behavior_switch', lexer.styles.keyword)

lexer.property['scintillua.comment'] = '<!--|-->'
lexer.property['scintillua.angle.braces'] = '1'

return lex
