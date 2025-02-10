-- Copyright 2006-2025 Mitchell. See LICENSE.
-- JSP LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {inherit = lexer.load('html')})

-- Embedded Java.
local java = lexer.load('java')
local java_start_rule = lex:tag(lexer.PREPROCESSOR, '<%' * P('=')^-1)
local java_end_rule = lex:tag(lexer.PREPROCESSOR, '%>')
lex:embed(java, java_start_rule, java_end_rule, true)

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, '<%', '%>')

lexer.property['scintillua.comment'] = '<!--|-->'

return lex
