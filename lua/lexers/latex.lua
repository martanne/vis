-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Latex LPeg lexer.
-- Modified by Brian Schott.
-- Modified by Robert Gieseke.

local lexer = lexer
local word_match = lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Comments.
local line_comment = lexer.to_eol('%')
local block_comment = lexer.range('\\begin' * P(' ')^0 * '{comment}',
	'\\end' * P(' ')^0 * '{comment}')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Math environments.
local math_word = word_match('align displaymath eqnarray equation gather math multline')
local math_begin_end = (P('begin') + P('end')) * P(' ')^0 * '{' * math_word * P('*')^-1 * '}'
lex:add_rule('math', lex:tag('environment.math', '$' + '\\' * (S('[]()') + math_begin_end)))

-- LaTeX environments.
lex:add_rule('environment', lex:tag('environment', '\\' * (P('begin') + 'end') * P(' ')^0 * '{' *
	lexer.word * P('*')^-1 * '}'))

-- Sections.
lex:add_rule('section', lex:tag('command.section', '\\' *
	word_match('part chapter section subsection subsubsection paragraph subparagraph') * P('*')^-1))

-- Commands.
lex:add_rule('command', lex:tag('command', '\\' * (lexer.alpha^1 + S('#$&~_^%{}\\'))))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('&#{}[]')))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '\\begin', '\\end')
lex:add_fold_point('environment', '\\begin', '\\end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

lexer.property['scintillua.comment'] = '%'

return lex
