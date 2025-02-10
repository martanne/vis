-- Copyright 2023-2025 Mitchell. See LICENSE.
-- troff/man LPeg lexer.
-- Based on original Man lexer by David B. Lamkins and modified by Eolien55.

local lexer = lexer
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local lex = lexer.new(...)

-- Registers and groff's structured programming.
lex:add_rule('keywords', lex:tag(lexer.KEYWORD, (lexer.starts_line('.') * (lexer.space - '\n')^0 *
	(P('while') + 'break' + 'continue' + 'nr' + 'rr' + 'rnn' + 'aln' + '\\}')) + '\\{'))

-- Markup.
lex:add_rule('escape_sequences', lex:tag(lexer.VARIABLE,
	'\\' * (('s' * S('+-')^-1) + S('*fgmnYV'))^-1 * (P('(') * 2 + lexer.range('[', ']') + 1)))

lex:add_rule('headings', lex:tag(lexer.NUMBER,
	lexer.starts_line('.') * (lexer.space - '\n')^0 * (S('STN') * 'H') * (lexer.space - '\n') *
		lexer.nonnewline^0))
lex:add_rule('man_alignment', lex:tag(lexer.KEYWORD,
	lexer.starts_line('.') * (lexer.space - '\n')^0 * (P('br') + 'DS' + 'RS' + 'RE' + 'PD' + 'PP') *
		lexer.space))
lex:add_rule('font', lex:tag(lexer.VARIABLE,
	lexer.starts_line('.') * (lexer.space - '\n')^0 * ('B' * P('R')^-1 + 'I' * S('PR')^-1) *
		lexer.space))

-- Lowercase troff macros are plain macros (like .so or .nr).
lex:add_rule('troff_plain_macros', lex:tag(lexer.VARIABLE, lexer.starts_line('.') *
	(lexer.space - '\n')^0 * lexer.lower^1))
lex:add_rule('any_macro', lex:tag(lexer.PREPROCESSOR,
	lexer.starts_line('.') * (lexer.space - '\n')^0 * (lexer.any - lexer.space)^0))
lex:add_rule('comment', lex:tag(lexer.COMMENT,
	(lexer.starts_line('.\\"') + '\\"' + '\\#') * lexer.nonnewline^0))
lex:add_rule('string', lex:tag(lexer.STRING, lexer.range('"', true)))

-- Usually used by eqn, and mandoc in some way.
lex:add_rule('in_dollars', lex:tag(lexer.EMBEDDED, lexer.range('$', false, false)))

-- TODO: a lexer for each preprocessor?

return lex
