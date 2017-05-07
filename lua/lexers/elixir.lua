-- Copyright 2015-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Contributed by Richard Philips.
-- Elixer LPeg lexer.

local l = require('lexer')
local token, style, color, word_match = l.token, l.style, l.color, l.word_match
local B, P, R, S = lpeg.B, lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'elixir'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline_esc^0)

-- Strings.
local dq_str = l.delimited_range('"', false)
local triple_dq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local string = token(l.STRING, triple_dq_str + dq_str)

-- Numbers
local dec = l.digit * (l.digit + P("_"))^0
local bin = '0b' * S('01')^1
local oct = '0o' * R('07')^1
local integer = bin + l.hex_num + oct + dec
local float = l.digit^1 * P(".") * l.digit^1 * S("eE") *
              (S('+-')^-1 * l.digit^1)^-1
local number_token = B(1 - R('az', 'AZ', '__')) *
                     (S('+-')^-1) * token(l.NUMBER, (float + integer))

-- Keywords.
local keyword_token = token(l.KEYWORD, word_match{
  "is_atom", "is_binary", "is_bitstring", "is_boolean", "is_float",
  "is_function", "is_integer", "is_list", "is_map", "is_number", "is_pid",
  "is_port", "is_record", "is_reference", "is_tuple", "is_exception", "case",
  "when", "cond", "for", "if", "unless", "try", "receive", "send", "exit",
  "raise", "throw", "after", "rescue", "catch", "else", "do", "end", "quote",
  "unquote", "super", "import", "require", "alias", "use", "self", "with", "fn"
})

-- Functions
local function_token = token(l.FUNCTION, word_match{
  "defstruct", "defrecordp", "defrecord", "defprotocol", "defp",
  "defoverridable", "defmodule", "defmacrop", "defmacro", "defimpl",
  "defexception", "defdelegate", "defcallback", "def"
})

-- Sigils
local sigil11 = P("~") * S("CRSW") * l.delimited_range('<>', false, true)
local sigil12 = P("~") * S("CRSW") * l.delimited_range('{}', false, true)
local sigil13 = P("~") * S("CRSW") * l.delimited_range('[]', false, true)
local sigil14 = P("~") * S("CRSW") * l.delimited_range('()', false, true)
local sigil15 = P("~") * S("CRSW") * l.delimited_range('|', false, true)
local sigil16 = P("~") * S("CRSW") * l.delimited_range('/', false, true)
local sigil17 = P("~") * S("CRSW") * l.delimited_range('"', false, true)
local sigil18 = P("~") * S("CRSW") * l.delimited_range("'", false, true)
local sigil19 = P("~") * S("CRSW") * '"""' * (l.any - '"""')^0 * P('"""')^-1
local sigil10 = P("~") * S("CRSW") * "'''" * (l.any - "'''")^0 * P("'''")^-1
local sigil21 = P("~") * S("crsw") * l.delimited_range('<>', false, false)
local sigil22 = P("~") * S("crsw") * l.delimited_range('{}', false, false)
local sigil23 = P("~") * S("crsw") * l.delimited_range('[]', false, false)
local sigil24 = P("~") * S("crsw") * l.delimited_range('()', false, false)
local sigil25 = P("~") * S("crsw") * l.delimited_range('|', false, false)
local sigil26 = P("~") * S("crsw") * l.delimited_range('/', false, false)
local sigil27 = P("~") * S("crsw") * l.delimited_range('"', false, false)
local sigil28 = P("~") * S("crsw") * l.delimited_range("'", false, false)
local sigil29 = P("~") * S("csrw") * '"""' * (l.any - '"""')^0 * P('"""')^-1
local sigil20 = P("~") * S("csrw") * "'''" * (l.any - "'''")^0 * P("'''")^-1
local sigil_token = token(l.REGEX, sigil10 + sigil19 + sigil11 + sigil12 +
                                   sigil13 + sigil14 + sigil15 + sigil16 +
                                   sigil17 + sigil18 + sigil20 + sigil29 +
                                   sigil21 + sigil22 + sigil23 + sigil24 +
                                   sigil25 + sigil26 + sigil27 + sigil28)
local sigiladdon_token = token(l.EMBEDDED, R('az', 'AZ')^0)

-- Attributes
local attribute_token = token(l.LABEL, B(1 - R('az', 'AZ', '__')) * P('@') *
                                       R('az','AZ') * R('az','AZ','09','__')^0)

-- Booleans
local boolean_token = token(l.NUMBER,
                            P(':')^-1 * word_match{"true", "false", "nil"})

-- Identifiers
local identifier = token(l.IDENTIFIER, R('az', '__') *
                                       R('az', 'AZ', '__', '09')^0 * S('?!')^-1)

-- Atoms
local atom1 = B(1 - P(':')) * P(':') * dq_str
local atom2 = B(1 - P(':')) * P(':') * R('az', 'AZ') *
              R('az', 'AZ', '__', '@@', '09')^0 * S('?!')^-1
local atom3 = B(1 - R('az', 'AZ', '__', '09', '::')) *
              R('AZ') * R('az', 'AZ', '__', '@@', '09')^0 * S('?!')^-1
local atom_token = token(l.CONSTANT, atom1 + atom2 + atom3)

-- Operators
local operator1 = word_match{"and", "or", "not", "when", "xor", "in"}
local operator2 = P('!==') + '!=' + '!' + '=~' + '===' + '==' + '=' + '<<<' +
                  '<<' + '<=' + '<-' + '<' + '>>>' + '>>' + '>=' + '>' + '->' +
                  '--' + '-' + '++' + '+' + '&&&' + '&&' + '&' + '|||' + '||' +
                  '|>' + '|' + '..' + '.' + '^^^' + '^' + '\\\\' + '::' + '*' +
                  '/' + '~~~' + '@'
local operator_token = token(l.OPERATOR, operator1 + operator2)

M._rules = {
  {'whitespace', ws},
  {'sigil', sigil_token * sigiladdon_token},
  {'atom', atom_token},
  {'string', string},
  {'comment', comment},
  {'attribute', attribute_token},
  {'boolean', boolean_token},
  {'function', function_token},
  {'keyword', keyword_token},
  {'operator', operator_token},
  {'identifier', identifier},
  {'number', number_token},
}

M._FOLDBYINDENTATION = true

return M
