-- Copyright 2014-2022 Joshua Krämer. See LICENSE.
-- Tcl LPeg lexer.
-- This lexer follows the TCL dodekalogue (http://wiki.tcl.tk/10259).
-- It is based on the previous lexer by Mitchell.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('tcl')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comment.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#' * P(function(input, index)
  local i = index - 2
  while i > 0 and input:find('^[ \t]', i) do i = i - 1 end
  if i < 1 or input:find('^[\r\n;]', i) then return index end
end))))

-- Separator (semicolon).
lex:add_rule('separator', token(lexer.CLASS, ';'))

-- Argument expander.
lex:add_rule('expander', token(lexer.LABEL, '{*}'))

-- Delimiters.
lex:add_rule('braces', token(lexer.KEYWORD, S('{}')))
lex:add_rule('quotes', token(lexer.FUNCTION, '"'))
lex:add_rule('brackets', token(lexer.VARIABLE, S('[]')))

-- Variable substitution.
lex:add_rule('variable', token(lexer.STRING, '$' * (lexer.alnum + '_' + P(':')^2)^0))

-- Backslash substitution.
local oct = lexer.digit * lexer.digit^-2
local hex = 'x' * lexer.xdigit^1
local unicode = 'u' * lexer.xdigit * lexer.xdigit^-3
lex:add_rule('backslash', token(lexer.TYPE, '\\' * (oct + hex + unicode + 1)))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
