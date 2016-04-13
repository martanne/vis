-- Taskpaper LPeg lexer
-- Copyright (c) 2016 Larry Hynes
-- See LICENSE
-- Improvements or suggestions to larry-at-larryhynes.com
-- Important Disclaimer:
-- I have no idea what I'm doing

local l = require('lexer')
local token = l.token
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'taskpaper'}

local delimiter = P('    ') + P('\t')

-- Whitespace
local ws = token(l.WHITESPACE, l.space^1)

-- Tags
local day_tag = token('day_tag',
        (P('@today') + P('@tomorrow')))

local overdue_tag = token('overdue_tag', P('@overdue'))
        
local plain_tag = token('plain_tag', P('@') * l.word)

local extended_tag = token('extended_tag',
        P('@') * l.word * P('(') * (l.word + R('09') + P('-'))^1 * P(')'))

-- Projects
local project = token('project',
        l.nested_pair(l.starts_line(l.alnum), ':') * l.newline)

-- Notes
local note = token('note',
        delimiter^1 * l.alnum * l.nonnewline^0)

-- Tasks
local task = token('task', delimiter^1 * P('-'))

M._rules = {
    {'note', note},
    {'task', task},
    {'project', project},
    {'extended_tag', extended_tag},
    {'day_tag', day_tag},
    {'overdue_tag', overdue_tag},
    {'plain_tag', plain_tag},
    {'whitespace', ws},
}

M._tokenstyles = {
    note = 'fore:cyan',
    task = 'fore:blue',
    project = 'fore:red',
    extended_tag = 'fore:blue',
    day_tag = 'fore:yellow',
    overdue_tag = 'fore:magenta',
    plain_tag = 'fore:blue',
}

M._LEXBYLINE = true

return M
