-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Erlang LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'erlang'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '%' * l.nonnewline^0)

-- Strings.
local string = token(l.STRING, l.delimited_range('"'))

-- Numbers.
local const_char = '$' * (('\\' * l.ascii) + l.any)
local number = token(l.NUMBER, const_char + l.float + l.integer)

-- Atoms.
local atom_pat = (l.lower * (l.alnum + '_')^0) + l.delimited_range("'")
local atom = token(l.LABEL, atom_pat)

-- Functions.
local func = token(l.FUNCTION, atom_pat * #l.delimited_range("()", false, false, true))

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'after', 'begin', 'case', 'catch', 'cond', 'end', 'fun', 'if', 'let', 'of',
  'query', 'receive', 'when'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, ((l.upper + '_') * (l.alnum + '_')^0))

-- Operators.
local named_operator = word_match{
  'div', 'rem', 'or', 'xor', 'bor', 'bxor', 'bsl', 'bsr', 'and', 'band', 'not',
  'bnot'
}
local operator = token(l.OPERATOR, S('-<>.;=/|#+*:,?!()[]{}') + named_operator)

-- Directives.
local directive = token('directive', '-' * word_match{
  'author', 'compile', 'copyright', 'define', 'doc', 'else', 'endif', 'export',
  'file', 'ifdef', 'ifndef', 'import', 'include_lib', 'include', 'module',
  'record', 'undef'
})

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'operator', operator},
  {'atom', atom},
  {'identifier', identifier},
  {'directive', directive},
  {'string', string},
  {'comment', comment},
  {'number', number}
}

M._tokenstyles = {
  directive = l.STYLE_PREPROCESSOR
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '[%(%)%[%]{}]', '%%'},
  [l.KEYWORD] = {
    case = 1, fun = 1, ['if'] = 1, query = 1, receive = 1, ['end'] = -1
  },
  [l.OPERATOR] = {
    ['('] = 1, [')'] = -1, ['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1
  },
  [l.COMMENT] = {['%'] = l.fold_line_comments('%')}
}

return M
