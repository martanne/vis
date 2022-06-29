-- Copyright 2015-2022 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- pure LPeg lexer, see http://purelang.bitbucket.org/

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('pure')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'namespace', 'with', 'end', 'using', 'interface', 'extern', 'let', 'const', 'def', 'type',
  'public', 'private', 'nonfix', 'outfix', 'infix', 'infixl', 'infixr', 'prefix', 'postfix', 'if',
  'otherwise', 'when', 'case', 'of', 'then', 'else'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true)))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
local bin = '0' * S('Bb') * S('01')^1
local hex = lexer.hex_num
local dec = lexer.dec_num
local int = (bin + hex + dec) * P('L')^-1
local rad = P('.') - '..'
local exp = (S('Ee') * S('+-')^-1 * int)^-1
local flt = int * (rad * dec)^-1 * exp + int^-1 * rad * dec * exp
lex:add_rule('number', token(lexer.NUMBER, flt + int))

-- Pragmas.
local hashbang = lexer.starts_line('#!') * (lexer.nonnewline - '//')^0
lex:add_rule('pragma', token(lexer.PREPROCESSOR, hashbang))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, '..' + S('+-/*%<>~!=^&|?~:;,.()[]{}@#$`\\\'')))

return lex
