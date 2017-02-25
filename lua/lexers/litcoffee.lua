-- Copyright 2006-2017 Robert Gieseke. See LICENSE.
-- Literate CoffeeScript LPeg lexer.
-- http://coffeescript.org/#literate

local l = require('lexer')
local token = l.token
local P = lpeg.P

local M = {_NAME = 'litcoffee'}

-- Embedded in Markdown.
local markdown = l.load('markdown')
M._lexer = markdown -- ensure markdown's rules are loaded, not HTML's

-- Embedded CoffeeScript.
local coffeescript = l.load('coffeescript')
local coffee_start_rule = token(l.STYLE_EMBEDDED, (P(' ')^4 + P('\t')))
local coffee_end_rule = token(l.STYLE_EMBEDDED, l.newline)
l.embed_lexer(markdown, coffeescript, coffee_start_rule, coffee_end_rule)

return M
