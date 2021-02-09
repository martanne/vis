-- Copyright 2016 Christian Hesse
-- delimiter separated values LPeg lexer.

local lexer = require('lexer')
local token = lexer.token
local S = lpeg.S

local lex = lexer.new('dsv')

lex:add_rule('operator', token(lexer.OPERATOR, S(',;:|')))

return lex
