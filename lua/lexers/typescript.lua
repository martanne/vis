-- Copyright 2021-2025 Mitchell. See LICENSE.
-- TypeScript LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {inherit = lexer.load('javascript')})

-- Word lists.
lex:set_word_list(lexer.KEYWORD, 'abstract as constructor declare is module namespace require type',
	true)

lex:set_word_list(lexer.TYPE, 'boolean number bigint string unknown any void never symbol object',
	true)

lexer.property['scintillua.comment'] = '//'

return lex
