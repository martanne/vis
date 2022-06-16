-- Copyright (c) 2016-2022 Larry Hynes. See LICENSE.
-- Taskpaper LPeg lexer

local lexer = require('lexer')
local token = lexer.token
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('taskpaper', {lex_by_line = true})

local delimiter = P('    ') + P('\t')

-- Notes.
lex:add_rule('note', token('note', delimiter^1 * lexer.to_eol(lexer.alnum)))
lex:add_style('note', lexer.styles.constant)

-- Tasks.
lex:add_rule('task', token('task', delimiter^1 * '-' + lexer.newline))
lex:add_style('task', lexer.styles['function'])

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Projects.
lex:add_rule('project',
  token('project', lexer.range(lexer.starts_line(lexer.alnum), ':') * lexer.newline))
lex:add_style('project', lexer.styles.label)

-- Tags.
lex:add_rule('extended_tag', token('extended_tag', '@' * lexer.word * '(' *
  (lexer.word + lexer.digit + '-')^1 * ')'))
lex:add_style('extended_tag', lexer.styles.comment)
lex:add_rule('day_tag', token('day_tag', (P('@today') + '@tomorrow')))
lex:add_style('day_tag', lexer.styles.class)
lex:add_rule('overdue_tag', token('overdue_tag', '@overdue'))
lex:add_style('overdue_tag', lexer.styles.preprocessor)
lex:add_rule('plain_tag', token('plain_tag', '@' * lexer.word))
lex:add_style('plain_tag', lexer.styles.comment)

return lex
