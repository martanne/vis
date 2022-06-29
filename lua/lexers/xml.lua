-- Copyright 2006-2022 Mitchell. See LICENSE.
-- XML LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('xml')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Comments and CDATA.
lex:add_rule('comment', token(lexer.COMMENT, lexer.range('<!--', '-->')))
lex:add_rule('cdata', token('cdata', lexer.range('<![CDATA[', ']]>')))
lex:add_style('cdata', lexer.styles.comment)

-- Doctypes and other markup tags.
local alpha = lpeg.R('az', 'AZ', '\127\255')
local word_char = lexer.alnum + S('_-:.??')
local identifier = (alpha + S('_-:.?')) * word_char^0
local doctype = token('doctype', '<!DOCTYPE') * ws * token('doctype', identifier) *
  (ws * identifier)^-1 * (1 - P('>'))^0 * token('doctype', '>')
lex:add_rule('doctype', doctype)
lex:add_style('doctype', lexer.styles.comment)

-- Processing instructions.
lex:add_rule('proc_insn', token('proc_insn', '<?' * (1 - P('?>'))^0 * P('?>')^-1))
lex:add_style('proc_insn', lexer.styles.comment)

-- Elements.
local namespace = token(lexer.OPERATOR, ':') * token('namespace', identifier)
lex:add_rule('element', token('element', '<' * P('/')^-1 * identifier) * namespace^-1)
lex:add_style('element', lexer.styles.keyword)
lex:add_style('namespace', lexer.styles.class)

-- Closing tags.
lex:add_rule('close_tag', token('element', P('/')^-1 * '>'))

-- Attributes.
lex:add_rule('attribute', token('attribute', identifier) * namespace^-1 * #(lexer.space^0 * '='))
lex:add_style('attribute', lexer.styles.type)

-- Equals.
-- TODO: performance is terrible on large files.
local in_tag = P(function(input, index)
  local before = input:sub(1, index - 1)
  local s, e = before:find('<[^>]-$'), before:find('>[^<]-$')
  if s and e then return s > e and index or nil end
  if s then return index end
  return input:find('^[^<]->', index) and index or nil
end)

-- lex:add_rule('equal', token(lexer.OPERATOR, '=')) -- * in_tag

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"', false, false)
lex:add_rule('string',
  #S('\'"') * lexer.last_char_includes('=') * token(lexer.STRING, sq_str + dq_str))

-- Numbers.
local number = token(lexer.NUMBER, lexer.dec_num * P('%')^-1)
lex:add_rule('number', #lexer.digit * lexer.last_char_includes('=') * number) -- *in_tag)

-- Entities.
lex:add_rule('entity', token('entity', '&' * word_match('lt gt amp apos quot') * ';'))
lex:add_style('entity', lexer.styles.operator)

-- Fold Points.
local function disambiguate_lt(text, pos, line, s) return not line:find('^</', s) and 1 or -1 end
lex:add_fold_point('element', '<', disambiguate_lt)
lex:add_fold_point('element', '/>', -1)
lex:add_fold_point(lexer.COMMENT, '<!--', '-->')
lex:add_fold_point('cdata', '<![CDATA[', ']]>')

return lex
