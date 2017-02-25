-- Copyright 2006-2017 Brian "Sir Alaran" Schott. See LICENSE.
-- Dot LPeg lexer.
-- Based off of lexer code by Mitchell.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'dot'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.digit^1 + l.float)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'graph', 'node', 'edge', 'digraph', 'fontsize', 'rankdir',
  'fontname', 'shape', 'label', 'arrowhead', 'arrowtail', 'arrowsize',
  'color', 'comment', 'constraint', 'decorate', 'dir', 'headlabel', 'headport',
  'headURL', 'labelangle', 'labeldistance', 'labelfloat', 'labelfontcolor',
  'labelfontname', 'labelfontsize', 'layer', 'lhead', 'ltail', 'minlen',
  'samehead', 'sametail', 'style', 'taillabel', 'tailport', 'tailURL', 'weight',
  'subgraph'
})

-- Types.
local type = token(l.TYPE, word_match{
	'box', 'polygon', 'ellipse', 'circle', 'point', 'egg', 'triangle',
	'plaintext', 'diamond', 'trapezium', 'parallelogram', 'house', 'pentagon',
	'hexagon', 'septagon', 'octagon', 'doublecircle', 'doubleoctagon',
	'tripleoctagon', 'invtriangle', 'invtrapezium', 'invhouse', 'Mdiamond',
	'Msquare', 'Mcircle', 'rect', 'rectangle', 'none', 'note', 'tab', 'folder',
	'box3d', 'record'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('->()[]{};'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'keyword', keyword},
  {'type', type},
  {'identifier', identifier},
  {'number', number},
  {'string', string},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
