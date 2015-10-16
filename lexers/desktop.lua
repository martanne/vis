-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Desktop Entry LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'desktop'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local string = token(l.STRING, l.delimited_range('"'))

-- Group headers.
local group_header = l.starts_line(token(l.STRING,
                                         l.delimited_range('[]', false, true)))

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer))

-- Keywords.
local keyword = token(l.KEYWORD, word_match{'true', 'false'})

-- Locales.
local locale = token(l.CLASS, l.delimited_range('[]', false, true))

-- Keys.
local key = token(l.VARIABLE, word_match{
  'Type', 'Version', 'Name', 'GenericName', 'NoDisplay', 'Comment', 'Icon',
  'Hidden', 'OnlyShowIn', 'NotShowIn', 'TryExec', 'Exec', 'Exec', 'Path',
  'Terminal', 'MimeType', 'Categories', 'StartupNotify', 'StartupWMClass', 'URL'
})

-- Field codes.
local code = l.token(l.CONSTANT, P('%') * S('fFuUdDnNickvm'))

-- Identifiers.
local identifier = l.token(l.IDENTIFIER, l.alpha * (l.alnum + S('_-'))^0)

-- Operators.
local operator = token(l.OPERATOR, S('='))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'key', key},
  {'identifier', identifier},
  {'group_header', group_header},
  {'locale', locale},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'code', code},
  {'operator', operator},
}

return M
