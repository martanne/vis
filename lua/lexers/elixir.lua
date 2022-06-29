-- Copyright 2015-2022 Mitchell. See LICENSE.
-- Contributed by Richard Philips.
-- Elixir LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local B, P, S = lpeg.B, lpeg.P, lpeg.S

local lex = lexer.new('elixir', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Sigils.
local sigil11 = '~' * S('CRSW') * lexer.range('<', '>')
local sigil12 = '~' * S('CRSW') * lexer.range('{', '}')
local sigil13 = '~' * S('CRSW') * lexer.range('[', ']')
local sigil14 = '~' * S('CRSW') * lexer.range('(', ')')
local sigil15 = '~' * S('CRSW') * lexer.range('|', false, false)
local sigil16 = '~' * S('CRSW') * lexer.range('/', false, false)
local sigil17 = '~' * S('CRSW') * lexer.range('"', false, false)
local sigil18 = '~' * S('CRSW') * lexer.range("'", false, false)
local sigil19 = '~' * S('CRSW') * lexer.range('"""')
local sigil10 = '~' * S('CRSW') * lexer.range("'''")
local sigil21 = '~' * S('crsw') * lexer.range('<', '>')
local sigil22 = '~' * S('crsw') * lexer.range('{', '}')
local sigil23 = '~' * S('crsw') * lexer.range('[', ']')
local sigil24 = '~' * S('crsw') * lexer.range('(', ')')
local sigil25 = '~' * S('crsw') * lexer.range('|')
local sigil26 = '~' * S('crsw') * lexer.range('/')
local sigil27 = '~' * S('crsw') * lexer.range('"')
local sigil28 = '~' * S('crsw') * lexer.range("'")
local sigil29 = '~' * S('crsw') * lexer.range('"""')
local sigil20 = '~' * S('crsw') * lexer.range("'''")
local sigil_token = token(lexer.REGEX,
  sigil10 + sigil19 + sigil11 + sigil12 + sigil13 + sigil14 + sigil15 + sigil16 + sigil17 + sigil18 +
    sigil20 + sigil29 + sigil21 + sigil22 + sigil23 + sigil24 + sigil25 + sigil26 + sigil27 +
    sigil28)
local sigiladdon_token = token(lexer.EMBEDDED, lexer.alpha^0)
lex:add_rule('sigil', sigil_token * sigiladdon_token)

-- Atoms.
local atom1 = B(1 - P(':')) * ':' * lexer.range('"')
local atom2 = B(1 - P(':')) * ':' * lexer.alpha * (lexer.alnum + S('_@'))^0 * S('?!')^-1
local atom3 = B(1 - (lexer.alnum + S('_:'))) * lexer.upper * (lexer.alnum + S('_@'))^0 * S('?!')^-1
lex:add_rule('atom', token(lexer.CONSTANT, atom1 + atom2 + atom3))

-- Strings.
local dq_str = lexer.range('"')
local triple_dq_str = lexer.range('"""')
lex:add_rule('string', token(lexer.STRING, triple_dq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#', true)))

-- Attributes.
lex:add_rule('attribute', token(lexer.LABEL, B(1 - (lexer.alnum + '_')) * '@' * lexer.alpha *
  (lexer.alnum + '_')^0))

-- Booleans.
lex:add_rule('boolean', token(lexer.NUMBER, P(':')^-1 * word_match('true false nil')))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'defstruct', 'defrecordp', 'defrecord', 'defprotocol', 'defp', 'defoverridable', 'defmodule',
  'defmacrop', 'defmacro', 'defimpl', 'defexception', 'defdelegate', 'defcallback', 'def'
}))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'is_atom', 'is_binary', 'is_bitstring', 'is_boolean', 'is_float', 'is_function', 'is_integer',
  'is_list', 'is_map', 'is_number', 'is_pid', 'is_port', 'is_record', 'is_reference', 'is_tuple',
  'is_exception', 'case', 'when', 'cond', 'for', 'if', 'unless', 'try', 'receive', 'send', 'exit',
  'raise', 'throw', 'after', 'rescue', 'catch', 'else', 'do', 'end', 'quote', 'unquote', 'super',
  'import', 'require', 'alias', 'use', 'self', 'with', 'fn'
}))

-- Operators
local operator1 = word_match('and or not when xor in')
local operator2 = P('!==') + '!=' + '!' + '=~' + '===' + '==' + '=' + '<<<' + '<<' + '<=' + '<-' +
  '<' + '>>>' + '>>' + '>=' + '>' + '->' + '--' + '-' + '++' + '+' + '&&&' + '&&' + '&' + '|||' +
  '||' + '|>' + '|' + '..' + '.' + '^^^' + '^' + '\\\\' + '::' + '*' + '/' + '~~~' + '@'
lex:add_rule('operator', token(lexer.OPERATOR, operator1 + operator2))

-- Identifiers
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word * S('?!')^-1))

-- Numbers
local dec = lexer.digit * (lexer.digit + '_')^0
local bin = '0b' * S('01')^1
local oct = '0o' * lpeg.R('07')^1
local integer = bin + lexer.hex_num + oct + dec
local float = lexer.digit^1 * '.' * lexer.digit^1 * S('eE') * (S('+-')^-1 * lexer.digit^1)^-1
lex:add_rule('number',
  B(1 - (lexer.alpha + '_')) * S('+-')^-1 * token(lexer.NUMBER, float + integer))

return lex
