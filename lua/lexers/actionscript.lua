-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Actionscript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('actionscript')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'break', 'continue', 'delete', 'do', 'else', 'for', 'function', 'if', 'in', 'new', 'on', 'return',
  'this', 'typeof', 'var', 'void', 'while', 'with', 'NaN', 'Infinity', 'false', 'null', 'true',
  'undefined',
  -- Reserved for future use.
  'abstract', 'case', 'catch', 'class', 'const', 'debugger', 'default', 'export', 'extends',
  'final', 'finally', 'goto', 'implements', 'import', 'instanceof', 'interface', 'native',
  'package', 'private', 'Void', 'protected', 'public', 'dynamic', 'static', 'super', 'switch',
  'synchonized', 'throw', 'throws', 'transient', 'try', 'volatile'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'Array', 'Boolean', 'Color', 'Date', 'Function', 'Key', 'MovieClip', 'Math', 'Mouse', 'Number',
  'Object', 'Selection', 'Sound', 'String', 'XML', 'XMLNode', 'XMLSocket',
  -- Reserved for future use.
  'boolean', 'byte', 'char', 'double', 'enum', 'float', 'int', 'long', 'short'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local ml_str = lexer.range('<![CDATA[', ']]>')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + ml_str))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('LlUuFf')^-2))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=!<>+-/*%&|^~.,;?()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))
lex:add_fold_point(lexer.STRING, '<![CDATA[', ']]>')

return lex
