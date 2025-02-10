-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Diff LPeg lexer.

local lexer = lexer
local to_eol, starts_line = lexer.to_eol, lexer.starts_line
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {lex_by_line = true})

-- Text, file headers, and separators.
lex:add_rule('index', lex:tag(lexer.COMMENT, to_eol(starts_line('Index: '))))
lex:add_rule('header', lex:tag(lexer.HEADING, to_eol(starts_line(P('*** ') + '--- ' + '+++ '))))
lex:add_rule('separator', lex:tag(lexer.COMMENT, to_eol(starts_line(P('---') + '****' + '='))))

-- Location.
lex:add_rule('location', lex:tag(lexer.NUMBER, to_eol(starts_line('@@' + lexer.dec_num + '****'))))

-- Additions, deletions, and changes.
lex:add_rule('addition', lex:tag('addition', to_eol(starts_line(S('>+')))))
lex:add_rule('deletion', lex:tag('deletion', to_eol(starts_line(S('<-')))))
lex:add_rule('change', lex:tag('change', to_eol(starts_line('!'))))

lex:add_rule('any_line', lex:tag(lexer.DEFAULT, lexer.to_eol()))

return lex
