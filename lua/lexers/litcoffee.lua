-- Copyright 2006-2022 Robert Gieseke. See LICENSE.
-- Literate CoffeeScript LPeg lexer.
-- http://coffeescript.org/#literate

local lexer = require('lexer')
local token = lexer.token
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('litcoffee', {inherit = lexer.load('markdown')})

-- Embedded CoffeeScript.
local coffeescript = lexer.load('coffeescript')
local coffee_start_rule = token(lexer.STYLE_EMBEDDED, (P(' ')^4 + P('\t')))
local coffee_end_rule = token(lexer.STYLE_EMBEDDED, lexer.newline)
lex:embed(coffeescript, coffee_start_rule, coffee_end_rule)

-- Use 'markdown_whitespace' instead of lexer.WHITESPACE since the latter would expand to
-- 'litcoffee_whitespace'.
lex:modify_rule('whitespace', token('markdown_whitespace', S(' \t')^1 + S('\r\n')^1))

return lex
