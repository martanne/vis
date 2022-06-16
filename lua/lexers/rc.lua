-- Copyright 2017-2022 Michael Forney. See LICENSE.
-- rc LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rc')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'for', 'in', 'while', 'if', 'not', 'switch', 'case', 'fn', 'builtin', 'cd', 'eval', 'exec',
  'exit', 'flag', 'rfork', 'shift', 'ulimit', 'umask', 'wait', 'whatis', '.', '~'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local str = lexer.range("'", false, false)
local heredoc = '<<' * P(function(input, index)
  local s, e, _, delimiter = input:find('[ \t]*(["\']?)([%w!"%%+,-./:?@_~]+)%1', index)
  if s == index and delimiter then
    delimiter = delimiter:gsub('[%%+-.?]', '%%%1')
    e = select(2, input:find('[\n\r]' .. delimiter .. '[\n\r]', e))
    return e and e + 1 or #input + 1
  end
end)
lex:add_rule('string', token(lexer.STRING, str + heredoc))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Variables.
lex:add_rule('variable',
  token(lexer.VARIABLE, '$' * S('"#')^-1 * ('*' + lexer.digit^1 + lexer.word)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('@`=!<>*&^|;?()[]{}') + '\\\n'))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
