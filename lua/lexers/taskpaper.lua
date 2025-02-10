-- Copyright (c) 2016-2025 Larry Hynes. See LICENSE.
-- Taskpaper LPeg lexer

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {lex_by_line = true})

-- Notes.
local delimiter = lpeg.B('    ') + lpeg.B('\t')
lex:add_rule('note', delimiter * lex:tag('note', lexer.to_eol(lexer.alnum)))

-- Tasks.
lex:add_rule('task', delimiter * lex:tag(lexer.LIST, '-'))

-- Projects.
lex:add_rule('project', lex:tag(lexer.HEADING,
	lexer.range(lexer.starts_line(lexer.alnum), ':') * lexer.newline))

-- Tags.
lex:add_rule('extended_tag', lex:tag(lexer.TAG .. '.extended', '@' * lexer.word * '(' *
	(lexer.word + lexer.digit + '-')^1 * ')'))
lex:add_rule('day_tag', lex:tag(lexer.TAG .. '.day', (P('@today') + '@tomorrow')))
lex:add_rule('overdue_tag', lex:tag(lexer.TAG .. '.overdue', '@overdue'))
lex:add_rule('plain_tag', lex:tag(lexer.TAG .. '.plain', '@' * lexer.word))

return lex
