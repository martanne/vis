-- Copyright 2017 Marc André Tanner
-- git-rebase(1) LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R = lpeg.P, lpeg.R

local M = {_NAME = 'git-rebase'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, l.starts_line('#') * l.nonnewline^0)

-- Keywords.
local keywords = l.starts_line(word_match{
  'p', 'pick',
  'r', 'reword',
  'e', 'edit',
  's', 'squash',
  'f', 'fixup',
  'x', 'exec',
  'd', 'drop',
  'l', 'label',
  't', 'reset',
  'm', 'merge',
})
local keyword = token(l.KEYWORD, keywords)

-- Commit SHA1.
local function patn(pat, min, max)
  return -pat^(max + 1) * pat^min
end

local commit = token(l.NUMBER, patn(R('09', 'af'), 7, 40))

local message = token(l.STRING, l.nonnewline^1)

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'keyword', keyword},
  {'commit', commit},
  {'message', message},
}

M._LEXBYLINE = true

return M
