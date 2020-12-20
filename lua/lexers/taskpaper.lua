-- Copyright (c) 2016-2020 Larry Hynes. See LICENSE.
-- Taskpaper LPeg lexer

local lexer = require('lexer')
local token = lexer.token
local P, S = lpeg.P, lpeg.S

local M = {_NAME = 'taskpaper'}

local delimiter = P('    ') + P('\t')

-- Whitespace
local ws = token(lexer.WHITESPACE, lexer.space^1)

-- Tags
local day_tag = token('day_tag', (P('@today') + P('@tomorrow')))

local overdue_tag = token('overdue_tag', P('@overdue'))

local plain_tag = token('plain_tag', P('@') * lexer.word)

local extended_tag = token('extended_tag', P('@') * lexer.word * P('(') *
  (lexer.word + lexer.digit + P('-'))^1 * P(')'))

-- Projects
local project = token('project', lexer.range(lexer.starts_line(lexer.alnum),
  ':', false, false, true) * lexer.newline)

-- Notes
local note = token('note', delimiter^1 * lexer.alnum * lexer.nonnewline^0)

-- Tasks
local task = token('task', delimiter^1 * P('-') + lexer.newline)

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
  note = lexer.STYLE_CONSTANT,
  task = lexer.STYLE_FUNCTION,
  project = lexer.STYLE_TAG,
  extended_tag = lexer.STYLE_COMMENT,
  day_tag = lexer.STYLE_CLASS,
  overdue_tag = lexer.STYLE_PREPROCESSOR,
  plain_tag = lexer.STYLE_COMMENT,
}

M._LEXBYLINE = true

return M
