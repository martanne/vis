-- Copyright 2006-2021 Mitchell. See LICENSE.
-- ? LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('?')

lex:add_rule('variable', token(lexer.VARIABLE, lexer.to_eol('#')))

-- Whitespace.
local ws = token(lexer.WHITESPACE, S(' \t')^1 + S('\v\r\n')^1)
lex:add_rule('whitespace', ws)

-- Span elements.
lex:add_rule('escape', token(lexer.DEFAULT, P('\\') * 1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  'set'
]]))

lex:add_rule('function', token(lexer.FUNCTION, word_match[[
  'text', 'page'
]]))


-- Headers.
local function h(n) return token('h' .. n, lexer.to_eol(lexer.starts_line(string.rep('=', n)))) end
lex:add_rule('header', h(6) + h(5) + h(4) + h(3) + h(2) + h(1))
local font_size = tonumber(lexer.property_expanded['style.default']:match('size:(%d+)')) or 10
local function add_header_style(n)
  lex:add_style('h' .. n, {fore = lexer.colors.red, size = (font_size + (6 - n))})
end
for i = 1, 6 do add_header_style(i) end

-- Lists.
lex:add_rule('list', token('list',
  lexer.starts_line(S(' \t')^0 * (S('+') + lexer.digit^1 * '.')) * S(' \t')))
lex:add_style('list', lexer.styles.constant)


-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))


-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'start', 'end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
