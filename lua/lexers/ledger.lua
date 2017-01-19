-- Copyright 2015-2017 Charles Lehner. See LICENSE.
-- ledger journal LPeg lexer, see http://www.ledger-cli.org/

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'ledger'}

local delim = P('\t') + P('  ')

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, S(';#') * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local label = l.delimited_range('[]', true, true)
local string = token(l.STRING, sq_str + dq_str + label)

-- Date.
local date = token(l.CONSTANT, l.starts_line((l.digit + S('/-')) ^1))

-- Account.
local account = token(l.VARIABLE,
                      l.starts_line(S(' \t')^1 * (l.print - delim)^1))

-- Amount.
local amount = token(l.NUMBER, delim * (1 - S(';\r\n'))^1)

-- Automated transactions.
local auto_tx = token(l.PREPROCESSOR, l.starts_line(S('=~') * l.nonnewline^0))

-- Directives.
local directive_word = word_match{
	'account', 'alias', 'assert', 'bucket', 'capture', 'check', 'comment',
	'commodity', 'define', 'end', 'fixed', 'endfixed', 'include', 'payee',
	'apply', 'tag', 'test', 'year'
} + S('AYNDCIiOobh')
local directive = token(l.KEYWORD, l.starts_line(S('!@')^-1 * directive_word))

M._rules = {
  {'account', account},
  {'amount', amount},
  {'comment', comment},
  {'whitespace', ws},
  {'date', date},
  {'auto_tx', auto_tx},
  {'directive', directive},
}

M._LEXBYLINE = true

return M
