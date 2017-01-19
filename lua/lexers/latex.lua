-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Latex LPeg lexer.
-- Modified by Brian Schott.
-- Modified by Robert Gieseke.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'latex'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '%' * l.nonnewline^0
local block_comment = '\\begin' * P(' ')^0 * '{comment}' *
                      (l.any - '\\end' * P(' ')^0 * '{comment}')^0 *
                      P('\\end' * P(' ')^0 * '{comment}')^-1
-- Note: need block_comment before line_comment or LPeg cannot compile rule.
local comment = token(l.COMMENT, block_comment + line_comment)

-- Sections.
local section = token('section', '\\' * word_match{
  'part', 'chapter', 'section', 'subsection', 'subsubsection', 'paragraph',
  'subparagraph'
} * P('*')^-1)

-- Math environments.
local math_word = word_match{
  'align', 'displaymath', 'eqnarray', 'equation', 'gather', 'math', 'multline'
}
local math_begin_end = (P('begin') + P('end')) * P(' ')^0 *
                       '{' * math_word * P('*')^-1 * '}'
local math = token('math', '$' + '\\' * (S('[]()') + math_begin_end))

-- LaTeX environments.
local environment = token('environment', '\\' * (P('begin') + P('end')) *
                                         P(' ')^0 *
                                         '{' * l.word * P('*')^-1 * '}')

-- Commands.
local command = token(l.KEYWORD, '\\' * (l.alpha^1 + S('#$&~_^%{}')))

-- Operators.
local operator = token(l.OPERATOR, S('&#{}[]'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'math', math},
  {'environment', environment},
  {'section', section},
  {'keyword', command},
  {'operator', operator},
}

M._tokenstyles = {
  environment = l.STYLE_KEYWORD,
  math = l.STYLE_FUNCTION,
  section = l.STYLE_CLASS
}

M._foldsymbols = {
  _patterns = {'\\[a-z]+', '[{}]', '%%'},
  [l.COMMENT] = {
    ['\\begin'] = 1, ['\\end'] = -1, ['%'] = l.fold_line_comments('%')
  },
  ['environment'] = {['\\begin'] = 1, ['\\end'] = -1},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1}
}

return M
