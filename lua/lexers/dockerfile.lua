-- Copyright 2016-2017 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- Dockerfile LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'dockerfile'}

-- Whitespace
local indent = #l.starts_line(S(' \t')) *
               (token(l.WHITESPACE, ' ') + token('indent_error', '\t'))^1
local ws = token(l.WHITESPACE, S(' \t')^1 + l.newline^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"')
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'ADD', 'ARG', 'CMD', 'COPY', 'ENTRYPOINT', 'ENV', 'EXPOSE', 'FROM', 'LABEL',
  'MAINTAINER', 'ONBUILD', 'RUN', 'STOPSIGNAL', 'USER', 'VOLUME', 'WORKDIR'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variable.
local variable = token(l.VARIABLE,
                       S('$')^1 * (S('{')^1 * l.word * S('}')^1 + l.word))

-- Operators.
local operator = token(l.OPERATOR, S('\\[],=:{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'variable', variable},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._FOLDBYINDENTATION = true

return M
