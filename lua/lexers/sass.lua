-- Copyright 2006-2022 Robert Gieseke. See LICENSE.
-- Sass CSS preprocessor LPeg lexer.
-- http://sass-lang.com

local lexer = require('lexer')
local token = lexer.token
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('sass', {inherit = lexer.load('css')})

-- Line comments.
lex:add_rule('line_comment', token(lexer.COMMENT, lexer.to_eol('//')))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, '$' * (lexer.alnum + S('_-'))^1))

-- Mixins.
lex:add_rule('mixin', token('mixin', '@' * lexer.word))
lex:add_style('mixin', lexer.styles['function'])

-- Fold points.
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
