-- Copyright 2006-2025 Robert Gieseke. See LICENSE.
-- Literate CoffeeScript LPeg lexer.
-- http://coffeescript.org/#literate

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {inherit = lexer.load('markdown')})

-- Distinguish between horizontal and vertical space so coffee_start_rule has a chance to match.
lex:modify_rule('whitespace', lex:tag(lexer.WHITESPACE, S(' \t')^1 + S('\r\n')^1))

-- Embedded CoffeeScript.
local coffeescript = lexer.load('coffeescript')
local coffee_start_rule = #(P(' ')^4 + P('\t')) * lex:get_rule('whitespace')
local coffee_end_rule = #lexer.newline * lex:get_rule('whitespace')
lex:embed(coffeescript, coffee_start_rule, coffee_end_rule)

return lex
