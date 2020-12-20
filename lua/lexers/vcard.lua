-- Copyright (c) 2015-2020 Piotr Orzechowski [drzewo.org]. See LICENSE.
-- vCard 2.1, 3.0 and 4.0 LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local M = {_NAME = 'vcard'}

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)

-- Required properties.
local required_property = token(lexer.KEYWORD, word_match({
  'BEGIN', 'END', 'FN', 'N' --[[ Not required in v4.0. ]], 'VERSION'
}, nil, true)) * #P(':')

-- Supported properties.
local supported_property = token(lexer.TYPE, word_match({
  'ADR', 'AGENT' --[[ Not supported in v4.0. ]],
  'ANNIVERSARY' --[[ Supported in v4.0 only. ]], 'BDAY',
  'CALADRURI' --[[ Supported in v4.0 only. ]],
  'CALURI' --[[ Supported in v4.0 only. ]], 'CATEGORIES',
  'CLASS' --[[ Supported in v3.0 only. ]],
  'CLIENTPIDMAP' --[[ Supported in v4.0 only. ]], 'EMAIL', 'END',
  'FBURL' --[[ Supported in v4.0 only. ]],
  'GENDER' --[[ Supported in v4.0 only. ]], 'GEO',
  'IMPP' --[[ Not supported in v2.1. ]], 'KEY',
  'KIND' --[[ Supported in v4.0 only. ]],
  'LABEL' --[[ Not supported in v4.0. ]],
  'LANG' --[[ Supported in v4.0 only. ]], 'LOGO',
  'MAILER' --[[ Not supported in v4.0. ]],
  'MEMBER' --[[ Supported in v4.0 only. ]],
  'NAME' --[[ Supported in v3.0 only. ]],
  'NICKNAME' --[[ Not supported in v2.1. ]], 'NOTE', 'ORG', 'PHOTO',
  'PRODID' --[[ Not supported in v2.1. ]],
  'PROFILE' --[[ Not supported in v4.0. ]],
  'RELATED' --[[ Supported in v4.0 only. ]], 'REV', 'ROLE',
  'SORT-STRING' --[[ Not supported in v4.0. ]], 'SOUND', 'SOURCE', 'TEL',
  'TITLE', 'TZ', 'UID', 'URL', 'XML' --[[ Supported in v4.0 only. ]]
}, nil, true)) * #S(':;')

local identifier = lexer.alpha^1 * lexer.digit^0 * (P('-') * lexer.alnum^1)^0

-- Extension.
local extension = token(lexer.TYPE, lexer.starts_line(S('xX') * P('-') *
  identifier * #S(':;')))

-- Parameter.
local parameter = token(lexer.IDENTIFIER,
  lexer.starts_line(identifier * #S(':='))) + token(lexer.STRING, identifier) *
  #S(':=')

-- Operators.
local operator = token(lexer.OPERATOR, S('.:;='))

-- Group and property.
local group_sequence = token(lexer.CONSTANT, lexer.starts_line(identifier)) *
  token(lexer.OPERATOR, P('.')) * (required_property + supported_property +
  lexer.token(lexer.TYPE, S('xX') * P('-') * identifier) * #S(':;'))
-- Begin vCard, end vCard.
local begin_sequence = token(lexer.KEYWORD, P('BEGIN')) *
  token(lexer.OPERATOR, P(':')) * token(lexer.COMMENT, P('VCARD'))
local end_sequence = token(lexer.KEYWORD, P('END')) *
  token(lexer.OPERATOR, P(':')) * token(lexer.COMMENT, P('VCARD'))

-- vCard version (in v3.0 and v4.0 must appear immediately after BEGIN:VCARD).
local version_sequence = token(lexer.KEYWORD, P('VERSION')) *
  token(lexer.OPERATOR, P(':')) *
  token(lexer.CONSTANT, lexer.digit^1 * (P('.') * lexer.digit^1)^-1)

-- Data.
local data = token(lexer.IDENTIFIER, lexer.any)

-- Rules.
M._rules = {
  {'whitespace', ws},
  {'begin_sequence', begin_sequence},
  {'end_sequence', end_sequence},
  {'version_sequence', version_sequence},
  {'group_sequence', group_sequence},
  {'required_property', required_property},
  {'supported_property', supported_property},
  {'extension', extension},
  {'parameter', parameter},
  {'operator', operator},
  {'data', data},
}

-- Folding.
M._foldsymbols = {
  _patterns = {'BEGIN', 'END'},
  [lexer.KEYWORD] = {['BEGIN'] = 1, ['END'] = -1}
}

return M
