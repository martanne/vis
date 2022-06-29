-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Diff LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('diff', {lex_by_line = true})

-- Text, separators, and file headers.
lex:add_rule('index', token(lexer.COMMENT, 'Index: ' * lexer.any^0 * -1))
lex:add_rule('separator', token(lexer.COMMENT, ('---' + P('*')^4 + P('=')^1) * lexer.space^0 * -1))
lex:add_rule('header', token('header', (P('*** ') + '--- ' + '+++ ') * lexer.any^1))
lex:add_style('header', lexer.styles.comment)

-- Location.
lex:add_rule('location', token(lexer.NUMBER, ('@@' + lexer.dec_num + '****') * lexer.any^1))

-- Additions, deletions, and changes.
lex:add_rule('addition', token('addition', S('>+') * lexer.any^0))
lex:add_style('addition', {fore = lexer.colors.green})
lex:add_rule('deletion', token('deletion', S('<-') * lexer.any^0))
lex:add_style('deletion', {fore = lexer.colors.red})
lex:add_rule('change', token('change', '!' * lexer.any^0))
lex:add_style('change', {fore = lexer.colors.yellow})

lex:add_rule('any_line', token(lexer.DEFAULT, lexer.any^1))

return lex
