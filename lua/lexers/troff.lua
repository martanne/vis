-- Copyright 2015-2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- man/roff LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'troff'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Markup.
local keywords = token(l.KEYWORD, (l.starts_line('.') * l.space^0 * (P('while') + P('break') + P('continue') + P('nr') + P('rr') + P('rnn') + P('aln') + P('\\}'))) + P('\\{'))

local escape_sequences = token(l.VARIABLE,
	P('\\') * ((P('s') * S('+-')) + S('*fgmnYV') + P("")) *
	(P('(') * 2 + P('[') * (l.any-P(']'))^0 * P(']') + 1))

local headings = token(l.NUMBER, P('.') * (S('STN') * P('H')) * (l.space + P("\n")) * l.nonnewline^0)
local alignment = token(l.KEYWORD,
                    (P('.br') + P('.DS') + P('.RS') + P('.RE') + P('.PD')) * (l.space + P("\n")))
local markup = token(l.VARIABLE,
                    (P('.B') * P('R')^-1 + P('.I') * S('PR')^-1 + P('.PP')) * (l.space + P("\n")))

local troff_plain_macros = token(l.VARIABLE, l.starts_line('.') * l.space^0 * l.lower^1)

local any_macro = token(l.PREPROCESSOR, l.starts_line('.') * l.space^0 * (l.any-l.space)^0)

local comment = token(l.COMMENT, (l.starts_line('.\\"') + P('\\"') + P('\\#')) * l.nonnewline^0)

local in_dollars = token(l.EMBEDDED, P('$') * (l.any - P('$'))^0 * P('$'))

local quoted = token(l.STRING, P('"') * (l.nonnewline-P('"'))^0 * (P('"') + (P("\n"))))

M._rules = {
  {'whitespace', ws},
  {'keywords', keywords},
  {'comment', comment},
  {'escape_sequences', escape_sequences},
  {'headings', headings},
  {'alignment', alignment},
  {'markup', markup},
  {'troff_plain_macros', troff_plain_macros},
  {'any_macro', any_macro},
  {'in_dollars', in_dollars},
  {'quoted', quoted},
}

return M
