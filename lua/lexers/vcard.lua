-- Copyright (c) 2015 Piotr Orzechowski [drzewo.org]. See LICENSE.
-- vCard 2.1, 3.0 and 4.0 LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'vcard'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Required properties.
local required_property = token(l.KEYWORD, word_match({
  'BEGIN', 'END', 'FN', 'N' --[[ Not required in v4.0. ]], 'VERSION'
}, nil, true)) * #P(':')

-- Supported properties.
local supported_property = token(l.TYPE, word_match({
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

local identifier = l.alpha^1 * l.digit^0 * (P('-') * l.alnum^1)^0

-- Extension.
local extension = token(l.TYPE,
                        l.starts_line(S('xX') * P('-') * identifier * #S(':;')))

-- Parameter.
local parameter = token(l.IDENTIFIER, l.starts_line(identifier * #S(':='))) +
                  token(l.STRING, identifier) * #S(':=')

-- Operators.
local operator = token(l.OPERATOR, S('.:;='))

-- Group and property.
local group_sequence = token(l.CONSTANT, l.starts_line(identifier)) *
                       token(l.OPERATOR, P('.')) *
                       (required_property + supported_property +
                        l.token(l.TYPE, S('xX') * P('-') * identifier) *
                       #S(':;'))
-- Begin vCard, end vCard.
local begin_sequence = token(l.KEYWORD, P('BEGIN')) *
                       token(l.OPERATOR, P(':')) * token(l.COMMENT, P('VCARD'))
local end_sequence = token(l.KEYWORD, P('END')) * token(l.OPERATOR, P(':')) *
                     token(l.COMMENT, P('VCARD'))

-- vCard version (in v3.0 and v4.0 must appear immediately after BEGIN:VCARD).
local version_sequence = token(l.KEYWORD, P('VERSION')) *
                         token(l.OPERATOR, P(':')) *
                         token(l.CONSTANT, l.digit^1 * (P('.') * l.digit^1)^-1)

-- Data.
local data = token(l.IDENTIFIER, l.any)

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
  [l.KEYWORD] = {['BEGIN'] = 1, ['END'] = -1}
}

return M
