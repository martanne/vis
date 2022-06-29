-- Copyright 2006-2022 Mitchell. See LICENSE.
-- F# LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('fsharp', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abstract', 'and', 'as', 'assert', 'asr', 'begin', 'class', 'default', 'delegate', 'do', 'done',
  'downcast', 'downto', 'else', 'end', 'enum', 'exception', 'false', 'finaly', 'for', 'fun',
  'function', 'if', 'in', 'iherit', 'interface', 'land', 'lazy', 'let', 'lor', 'lsl', 'lsr', 'lxor',
  'match', 'member', 'mod', 'module', 'mutable', 'namespace', 'new', 'null', 'of', 'open', 'or',
  'override', 'sig', 'static', 'struct', 'then', 'to', 'true', 'try', 'type', 'val', 'when',
  'inline', 'upcast', 'while', 'with', 'async', 'atomic', 'break', 'checked', 'component', 'const',
  'constructor', 'continue', 'eager', 'event', 'external', 'fixed', 'functor', 'include', 'method',
  'mixin', 'process', 'property', 'protected', 'public', 'pure', 'readonly', 'return', 'sealed',
  'switch', 'virtual', 'void', 'volatile', 'where',
  -- Booleans.
  'true', 'false'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'bool', 'byte', 'sbyte', 'int16', 'uint16', 'int', 'uint32', 'int64', 'uint64', 'nativeint',
  'unativeint', 'char', 'string', 'decimal', 'unit', 'void', 'float32', 'single', 'float', 'double'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('(*', '*)', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.float + lexer.integer * S('uUlL')^-1))

-- Preprocessor.
lex:add_rule('preproc', token(lexer.PREPROCESSOR, lexer.starts_line('#') * S('\t ')^0 *
  word_match('else endif endregion if ifdef ifndef light region')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=<>+-*/^.,:;~!@#%^&|?[](){}')))

return lex
