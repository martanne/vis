-- Copyright 2006-2020 Mitchell. See LICENSE.
-- Text LPeg lexer.

local lexer = require('lexer')

local lex = lexer.new('text')

lex:add_rule('whitespace', lexer.token(lexer.WHITESPACE, lexer.space^1))

return lex
