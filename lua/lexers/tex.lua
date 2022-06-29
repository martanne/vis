-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Plain TeX LPeg lexer.
-- Modified by Robert Gieseke.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('tex')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('%')))

-- TeX environments.
lex:add_rule('environment', token('environment', '\\' * (P('begin') + 'end') * lexer.word))
lex:add_style('environment', lexer.styles.keyword)

-- Commands.
lex:add_rule('command', token(lexer.KEYWORD, '\\' * (lexer.alpha^1 + S('#$&~_^%{}'))))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('$&#{}[]')))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('%'))
lex:add_fold_point('environment', '\\begin', '\\end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

return lex
