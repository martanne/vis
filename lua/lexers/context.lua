-- Copyright 2006-2022 Robert Gieseke, Lars Otter. See LICENSE.
-- ConTeXt LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('context')

-- TeX and ConTeXt mkiv environment definitions.
local beginend = (P('begin') + 'end')
local startstop = (P('start') + 'stop')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('%')))

-- Sections.
local wm_section = word_match{
  'chapter', 'part', 'section', 'subject', 'subsection', 'subsubject', 'subsubsection',
  'subsubsubject', 'subsubsubsection', 'subsubsubsubject', 'title'
}
local section = token(lexer.CLASS, '\\' * startstop^-1 * wm_section)
lex:add_rule('section', section)

-- TeX and ConTeXt mkiv environments.
local environment = token(lexer.STRING, '\\' * (beginend + startstop) * lexer.alpha^1)
lex:add_rule('environment', environment)

-- Commands.
local command = token(lexer.KEYWORD, '\\' *
  (lexer.alpha^1 * P('\\') * lexer.space^1 + lexer.alpha^1 + S('!"#$%&\',./;=[\\]_{|}~`^-')))
lex:add_rule('command', command)

-- Operators.
local operator = token(lexer.OPERATOR, S('#$_[]{}~^'))
lex:add_rule('operator', operator)

-- Fold points.
lex:add_fold_point('environment', '\\start', '\\stop')
lex:add_fold_point('environment', '\\begin', '\\end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('%'))

-- Embedded Lua.
local luatex = lexer.load('lua')
local luatex_start_rule = #P('\\startluacode') * environment
local luatex_end_rule = #P('\\stopluacode') * environment
lex:embed(luatex, luatex_start_rule, luatex_end_rule)

return lex
