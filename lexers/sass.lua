-- Copyright 2006-2013 Robert Gieseke. See LICENSE.
-- Sass CSS preprocessor LPeg lexer.
-- http://sass-lang.com

local l = require('lexer')
local token = l.token
local P, S = lpeg.P, lpeg.S

local M = {_NAME = 'sass'}

-- Line comments.
local line_comment = token(l.COMMENT, '//' * l.nonnewline^0)

-- Variables.
local variable = token(l.VARIABLE, '$' * (l.alnum + S('_-'))^1)

-- Mixins.
local mixin = token('mixin', P('@') * l.word)

local css = l.load('css')
local _rules = css._rules
table.insert(_rules, #_rules - 1, {'mixin', mixin})
table.insert(_rules, #_rules - 1, {'line_comment', line_comment})
table.insert(_rules, #_rules - 1, {'variable', variable})
M._rules = _rules

M._tokenstyles = css._tokenstyles
M._tokenstyles['mixin'] = l.STYLE_FUNCTION

M._foldsymbols = css._foldsymbols

return M
