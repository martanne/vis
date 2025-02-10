-- Copyright 2017-2025 Marc André Tanner. See LICENSE.
-- git-rebase(1) LPeg lexer.

local lexer = lexer
local P, R = lpeg.P, lpeg.R

local lex = lexer.new(..., {lex_by_line = true})

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol(lexer.starts_line('#'))))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lexer.starts_line(lex:word_match(lexer.KEYWORD))))

-- Commit SHA1.
local function patn(pat, min, max)
	return -pat^(max + 1) * pat^min
end

lex:add_rule('commit', lex:tag(lexer.NUMBER, patn(R('09', 'af'), 7, 40)))

lex:add_rule('message', lex:tag('message', lexer.to_eol()))

-- Word lists.
lex:set_word_list(lexer.KEYWORD, [[
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

return lex
