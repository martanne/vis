-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Pascal LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'pascal'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local bblock_comment = '{' * (l.any - '}')^0 * P('}')^-1
local pblock_comment = '(*' * (l.any - '*)')^0 * P('*)')^-1
local comment = token(l.COMMENT, line_comment + bblock_comment + pblock_comment)

-- Strings.
local string = token(l.STRING, S('uUrR')^-1 *
                               l.delimited_range("'", true, true))

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('LlDdFf')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'and', 'array', 'as', 'at', 'asm', 'begin', 'case', 'class', 'const',
  'constructor', 'destructor', 'dispinterface', 'div', 'do', 'downto', 'else',
  'end', 'except', 'exports', 'file', 'final', 'finalization', 'finally', 'for',
  'function', 'goto', 'if', 'implementation', 'in', 'inherited',
  'initialization', 'inline', 'interface', 'is', 'label', 'mod', 'not',
  'object', 'of', 'on', 'or', 'out', 'packed', 'procedure', 'program',
  'property', 'raise', 'record', 'repeat', 'resourcestring', 'set', 'sealed',
  'shl', 'shr', 'static', 'string', 'then', 'threadvar', 'to', 'try', 'type',
  'unit', 'unsafe', 'until', 'uses', 'var', 'while', 'with', 'xor',
  'absolute', 'abstract', 'assembler', 'automated', 'cdecl', 'contains',
  'default', 'deprecated', 'dispid', 'dynamic', 'export', 'external', 'far',
  'forward', 'implements', 'index', 'library', 'local', 'message', 'name',
  'namespaces', 'near', 'nodefault', 'overload', 'override', 'package',
  'pascal', 'platform', 'private', 'protected', 'public', 'published', 'read',
  'readonly', 'register', 'reintroduce', 'requires', 'resident', 'safecall',
  'stdcall', 'stored', 'varargs', 'virtual', 'write', 'writeln', 'writeonly',
  'false', 'nil', 'self', 'true'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, word_match({
  'chr', 'ord', 'succ', 'pred', 'abs', 'round', 'trunc', 'sqr', 'sqrt',
  'arctan', 'cos', 'sin', 'exp', 'ln', 'odd', 'eof', 'eoln'
}, nil, true))

-- Types.
local type = token(l.TYPE, word_match({
  'shortint', 'byte', 'char', 'smallint', 'integer', 'word', 'longint',
  'cardinal', 'boolean', 'bytebool', 'wordbool', 'longbool', 'real', 'single',
  'double', 'extended', 'comp', 'currency', 'pointer'
}, nil, true))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('.,;^@:=<>+-/*()[]'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'type', type},
  {'string', string},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
