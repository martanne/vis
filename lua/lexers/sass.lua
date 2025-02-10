-- Copyright 2006-2025 Robert Gieseke. See LICENSE.
-- Sass CSS preprocessor LPeg lexer.
-- http://sass-lang.com

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {inherit = lexer.load('css')})

-- Line comments.
lex:add_rule('line_comment', lex:tag(lexer.COMMENT, lexer.to_eol('//')))

-- Variables.
lex:add_rule('variable', lex:tag(lexer.VARIABLE, '$' * (lexer.alnum + S('_-'))^1))

-- Mixins.
lex:add_rule('mixin', lex:tag(lexer.PREPROCESSOR, '@' * lexer.word))

lexer.property['scintillua.comment'] = '//'

return lex
