-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Latex LPeg lexer.
-- Modified by Brian Schott.
-- Modified by Robert Gieseke.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('latex')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
local line_comment = lexer.to_eol('%')
local block_comment = lexer.range('\\begin' * P(' ')^0 * '{comment}',
  '\\end' * P(' ')^0 * '{comment}')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Math environments.
local math_word = word_match('align displaymath eqnarray equation gather math multline')
local math_begin_end = (P('begin') + P('end')) * P(' ')^0 * '{' * math_word * P('*')^-1 * '}'
lex:add_rule('math', token('math', '$' + '\\' * (S('[]()') + math_begin_end)))
lex:add_style('math', lexer.styles['function'])

-- LaTeX environments.
lex:add_rule('environment', token('environment', '\\' * (P('begin') + 'end') * P(' ')^0 * '{' *
  lexer.word * P('*')^-1 * '}'))
lex:add_style('environment', lexer.styles.keyword)

-- Sections.
lex:add_rule('section', token('section', '\\' *
  word_match('part chapter section subsection subsubsection paragraph subparagraph') * P('*')^-1))
lex:add_style('section', lexer.styles.class)

-- Commands.
lex:add_rule('command', token('command', '\\' * (lexer.alpha^1 + S('#$&~_^%{}\\'))))
lex:add_style('command', lexer.styles.keyword)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('&#{}[]')))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '\\begin', '\\end')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('%'))
lex:add_fold_point('environment', '\\begin', '\\end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

return lex
