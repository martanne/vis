-- Copyright (c) 2015-2021 Piotr Orzechowski [drzewo.org]. See LICENSE.
-- vCard 2.1, 3.0 and 4.0 LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('vcard')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Begin vCard, end vCard.
lex:add_rule('begin_sequence', token(lexer.KEYWORD, P('BEGIN')) *
  token(lexer.OPERATOR, P(':')) * token(lexer.COMMENT, P('VCARD')))
lex:add_rule('end_sequence', token(lexer.KEYWORD, P('END')) *
  token(lexer.OPERATOR, P(':')) * token(lexer.COMMENT, P('VCARD')))

-- vCard version (in v3.0 and v4.0 must appear immediately after BEGIN:VCARD).
lex:add_rule('version_sequence', token(lexer.KEYWORD, P('VERSION')) *
  token(lexer.OPERATOR, P(':')) *
  token(lexer.CONSTANT, lexer.digit^1 * ('.' * lexer.digit^1)^-1))

-- Required properties.
local required_property = token(lexer.KEYWORD, word_match([[
  BEGIN END FN VERSION
  N -- Not required in v4.0.
]], true)) * #P(':')
lex:add_rule('required_property', required_property)

-- Supported properties.
local supported_property = token(lexer.TYPE, word_match([[
  ADR BDAY CATEGORIES EMAIL END GEO KEY LOGO NOTE ORG PHOTO REV ROLE SOUND
  SOURCE TEL TITLE TZ UID URL
  -- Supported in v4.0 only.
  ANNIVERSARY CALADRURI CALURI CLIENTPIDMAP FBURL GENDER KIND LANG MEMBER
  RELATED XML
  -- Not supported in v4.0.
  AGENT LABEL MAILER PROFILE SORT-STRING
  -- Supported in v3.0 only.
  CLASS NAME
  -- Not supported in v2.1.
  IMPP NICKNAME PRODID
]], true)) * #S(':;')
lex:add_rule('supported_property', supported_property)

local identifier = lexer.alpha^1 * lexer.digit^0 * ('-' * lexer.alnum^1)^0

-- Group and property.
lex:add_rule('group_sequence',
  token(lexer.CONSTANT, lexer.starts_line(identifier)) *
  token(lexer.OPERATOR, P('.')) *
  (required_property + supported_property +
    lexer.token(lexer.TYPE, S('xX') * '-' * identifier) * #S(':;')))

-- Extension.
lex:add_rule('extension', token(lexer.TYPE, lexer.starts_line(S('xX') * '-' *
  identifier * #S(':;'))))

-- Parameter.
lex:add_rule('parameter',
  token(lexer.IDENTIFIER, lexer.starts_line(identifier * #S(':='))) +
  token(lexer.STRING, identifier) * #S(':='))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('.:;=')))

-- Data.
lex:add_rule('data', token(lexer.IDENTIFIER, lexer.any))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'BEGIN', 'END')

return lex
