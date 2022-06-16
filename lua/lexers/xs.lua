-- Copyright 2017-2022 David B. Lamkins. See LICENSE.
-- xs LPeg lexer.
-- Adapted from rc lexer by Michael Forney.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('xs')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'access', 'alias', 'catch', 'cd', 'dirs', 'echo', 'else', 'escape', 'eval', 'exec', 'exit',
  'false', 'fn-', 'fn', 'for', 'forever', 'fork', 'history', 'if', 'jobs', 'let', 'limit', 'local',
  'map', 'omap', 'popd', 'printf', 'pushd', 'read', 'result', 'set-', 'switch', 'throw', 'time',
  'true', 'umask', 'until', 'unwind-protect', 'var', 'vars', 'wait', 'whats', 'while', ':lt', ':le',
  ':gt', ':ge', ':eq', ':ne', '~', '~~', '...', '.'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local str = lexer.range("'", false, true)
local herestr = '<<<' * str
local heredoc = '<<' * P(function(input, index)
  local s, e, _, delimiter = input:find('[ \t]*(["\']?)([%w!"%%+,-./:?@_~]+)%1', index)
  if s == index and delimiter then
    delimiter = delimiter:gsub('[%%+-.?]', '%%%1')
    e = select(2, input:find('[\n\r]' .. delimiter .. '[\n\r]', e))
    return e and e + 1 or #input + 1
  end
end)
lex:add_rule('string', token(lexer.STRING, str + herestr + heredoc))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
-- lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, '$&' * lexer.word))

-- Variables.
lex:add_rule('variable',
  token(lexer.VARIABLE, '$' * S('"#')^-1 * ('*' + lexer.digit^1 + lexer.word)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('@`=!<>*&^|;?()[]{}') + '\\\n'))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
