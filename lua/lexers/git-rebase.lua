-- Copyright 2017-2021 Marc André Tanner
-- git-rebase(1) LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, R = lpeg.P, lpeg.R

local lex = lexer.new('git-rebase', {lex_by_line = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.starts_line('#') * lexer.nonnewline^0))

-- Keywords.
local keywords = lexer.starts_line(word_match[[
  p pick
  r reword
  e edit
  s squash
  f fixup
  x exec
  d drop
  b break
  l label
  t reset
  m merge
]])
lex:add_rule('keyword', token(lexer.KEYWORD, keywords))

-- Commit SHA1.
local function patn(pat, min, max)
  return -pat^(max + 1) * pat^min
end

lex:add_rule('commit', token(lexer.NUMBER, patn(R('09', 'af'), 7, 40)))

lex:add_rule('message', token(lexer.STRING, lexer.nonnewline^1))

return lex
