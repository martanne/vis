-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Pascal LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('pascal')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'and', 'array', 'as', 'at', 'asm', 'begin', 'case', 'class', 'const', 'constructor', 'destructor',
  'dispinterface', 'div', 'do', 'downto', 'else', 'end', 'except', 'exports', 'file', 'final',
  'finalization', 'finally', 'for', 'function', 'goto', 'if', 'implementation', 'in', 'inherited',
  'initialization', 'inline', 'interface', 'is', 'label', 'mod', 'not', 'object', 'of', 'on', 'or',
  'out', 'packed', 'procedure', 'program', 'property', 'raise', 'record', 'repeat',
  'resourcestring', 'set', 'sealed', 'shl', 'shr', 'static', 'string', 'then', 'threadvar', 'to',
  'try', 'type', 'unit', 'unsafe', 'until', 'uses', 'var', 'while', 'with', 'xor', 'absolute',
  'abstract', 'assembler', 'automated', 'cdecl', 'contains', 'default', 'deprecated', 'dispid',
  'dynamic', 'export', 'external', 'far', 'forward', 'implements', 'index', 'library', 'local',
  'message', 'name', 'namespaces', 'near', 'nodefault', 'overload', 'override', 'package', 'pascal',
  'platform', 'private', 'protected', 'public', 'published', 'read', 'readonly', 'register',
  'reintroduce', 'requires', 'resident', 'safecall', 'stdcall', 'stored', 'varargs', 'virtual',
  'write', 'writeln', 'writeonly', --
  'false', 'nil', 'self', 'true'
}, true)))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match({
  'chr', 'ord', 'succ', 'pred', 'abs', 'round', 'trunc', 'sqr', 'sqrt', 'arctan', 'cos', 'sin',
  'exp', 'ln', 'odd', 'eof', 'eoln'
}, true)))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match({
  'shortint', 'byte', 'char', 'smallint', 'integer', 'word', 'longint', 'cardinal', 'boolean',
  'bytebool', 'wordbool', 'longbool', 'real', 'single', 'double', 'extended', 'comp', 'currency',
  'pointer'
}, true)))

-- Strings.
lex:add_rule('string', token(lexer.STRING, S('uUrR')^-1 * lexer.range("'", true, false)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local bblock_comment = lexer.range('{', '}')
local pblock_comment = lexer.range('(*', '*)')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + bblock_comment + pblock_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('LlDdFf')^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('.,;^@:=<>+-/*()[]')))

return lex
