-- Copyright 2015-2022 Charles Lehner. See LICENSE.
-- ledger journal LPeg lexer, see http://www.ledger-cli.org/

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ledger', {lex_by_line = true})

local delim = P('\t') + P('  ')

-- Account.
lex:add_rule('account', token(lexer.VARIABLE, lexer.starts_line(S(' \t')^1 * lexer.graph^1)))

-- Amount.
lex:add_rule('amount', token(lexer.NUMBER, delim * (1 - S(';\r\n'))^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol(S(';#'))))

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local label = lexer.range('[', ']', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + label))

-- Date.
lex:add_rule('date', token(lexer.CONSTANT, lexer.starts_line((lexer.digit + S('/-'))^1)))

-- Automated transactions.
lex:add_rule('auto_tx', token(lexer.PREPROCESSOR, lexer.to_eol(lexer.starts_line(S('=~')))))

-- Directives.
local directive_word = word_match{
  '	account', 'alias', 'assert', 'bucket', 'capture', 'check', 'comment', 'commodity', 'define',
  'end', 'fixed', 'endfixed', 'include', 'payee', 'apply', 'tag', 'test', 'year'
} + S('AYNDCIiOobh')
lex:add_rule('directive', token(lexer.KEYWORD, lexer.starts_line(S('!@')^-1 * directive_word)))

return lex
