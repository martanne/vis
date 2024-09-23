-- Copyright 2024 elb. See LICENSE.
-- Mail LPeg lexer

local lexer = lexer
local lex = lexer.new(...)

lex:add_rule('quote', lex:tag(lexer.QUOTE, lexer.to_eol(lexer.starts_line('>'))))

return lex