-- Copyright 2006-2022 Mitchell. See LICENSE.
-- JSP LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('jsp', {inherit = lexer.load('html')})

-- Embedded Java.
local java = lexer.load('java')
local java_start_rule = token('jsp_tag', '<%' * P('=')^-1)
local java_end_rule = token('jsp_tag', '%>')
lex:embed(java, java_start_rule, java_end_rule, true)
lex:add_style('jsp_tag', lexer.styles.embedded)

-- Fold points.
lex:add_fold_point('jsp_tag', '<%', '%>')

return lex
