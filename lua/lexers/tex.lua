-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Plain TeX LPeg lexer.
-- Modified by Robert Gieseke.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('%')))

-- TeX environments.
lex:add_rule('environment', lex:tag('environment', '\\' * (P('begin') + 'end') * lexer.word))

-- Commands.
lex:add_rule('command', lex:tag('command', '\\' * (lexer.alpha^1 + S('#$&~_^%{}'))))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('$&#{}[]')))

-- Fold points.
lex:add_fold_point('environment', '\\begin', '\\end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

lexer.property['scintillua.comment'] = '%'

return lex
