-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Gnuplot LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'gnuplot'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local bk_str = l.delimited_range('[]', true)
local bc_str = l.delimited_range('{}', true)
local string = token(l.STRING, sq_str + dq_str + bk_str + bc_str)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'cd', 'call', 'clear', 'exit', 'fit', 'help', 'history', 'if', 'load',
  'pause', 'plot', 'using', 'with', 'index', 'every', 'smooth', 'thru', 'print',
  'pwd', 'quit', 'replot', 'reread', 'reset', 'save', 'set', 'show', 'unset',
  'shell', 'splot', 'system', 'test', 'unset', 'update'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'abs', 'acos', 'acosh', 'arg', 'asin', 'asinh', 'atan', 'atan2', 'atanh',
  'besj0', 'besj1', 'besy0', 'besy1', 'ceil', 'cos', 'cosh', 'erf', 'erfc',
  'exp', 'floor', 'gamma', 'ibeta', 'inverf', 'igamma', 'imag', 'invnorm',
  'int', 'lambertw', 'lgamma', 'log', 'log10', 'norm', 'rand', 'real', 'sgn',
  'sin', 'sinh', 'sqrt', 'tan', 'tanh', 'column', 'defined', 'tm_hour',
  'tm_mday', 'tm_min', 'tm_mon', 'tm_sec', 'tm_wday', 'tm_yday', 'tm_year',
  'valid'
})

-- Variables.
local variable = token(l.VARIABLE, word_match{
  'angles', 'arrow', 'autoscale', 'bars', 'bmargin', 'border', 'boxwidth',
  'clabel', 'clip', 'cntrparam', 'colorbox', 'contour', 'datafile ',
  'decimalsign', 'dgrid3d', 'dummy', 'encoding', 'fit', 'fontpath', 'format',
  'functions', 'function', 'grid', 'hidden3d', 'historysize', 'isosamples',
  'key', 'label', 'lmargin', 'loadpath', 'locale', 'logscale', 'mapping',
  'margin', 'mouse', 'multiplot', 'mx2tics', 'mxtics', 'my2tics', 'mytics',
  'mztics', 'offsets', 'origin', 'output', 'parametric', 'plot', 'pm3d',
  'palette', 'pointsize', 'polar', 'print', 'rmargin', 'rrange', 'samples',
  'size', 'style', 'surface', 'terminal', 'tics', 'ticslevel', 'ticscale',
  'timestamp', 'timefmt', 'title', 'tmargin', 'trange', 'urange', 'variables',
  'version', 'view', 'vrange', 'x2data', 'x2dtics', 'x2label', 'x2mtics',
  'x2range', 'x2tics', 'x2zeroaxis', 'xdata', 'xdtics', 'xlabel', 'xmtics',
  'xrange', 'xtics', 'xzeroaxis', 'y2data', 'y2dtics', 'y2label', 'y2mtics',
  'y2range', 'y2tics', 'y2zeroaxis', 'ydata', 'ydtics', 'ylabel', 'ymtics',
  'yrange', 'ytics', 'yzeroaxis', 'zdata', 'zdtics', 'cbdata', 'cbdtics',
  'zero', 'zeroaxis', 'zlabel', 'zmtics', 'zrange', 'ztics', 'cblabel',
  'cbmtics', 'cbrange', 'cbtics'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('-+~!$*%=<>&|^?:()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'variable', variable},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'operator', operator},
}

return M
