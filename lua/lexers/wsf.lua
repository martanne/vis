-- Copyright 2006-2022 Mitchell. See LICENSE.
-- WSF LPeg lexer (based on XML).
-- Contributed by Jeff Stone.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('wsf')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.range('<!--', '-->')))

-- Elements.
local alpha = lpeg.R('az', 'AZ', '\127\255')
local word_char = lexer.alnum + S('_-:.?')
local identifier = (alpha + S('_-:.?')) * word_char^0
local element = token('element', '<' * P('/')^-1 * identifier)
lex:add_rule('element', element)
lex:add_style('element', lexer.styles.keyword)

-- Closing tags.
local tag_close = token('element', P('/')^-1 * '>')
lex:add_rule('tag_close', tag_close)

-- Attributes.
local attribute = token('attribute', identifier) * #(lexer.space^0 * '=')
lex:add_rule('attribute', attribute)
lex:add_style('attribute', lexer.styles.type)

-- Equals.
local in_tag = P(function(input, index)
  local before = input:sub(1, index - 1)
  local s, e = before:find('<[^>]-$'), before:find('>[^<]-$')
  if s and e then return s > e and index or nil end
  if s then return index end
  return input:find('^[^<]->', index) and index or nil
end)

local equals = token(lexer.OPERATOR, '=') * in_tag
lex:add_rule('equals', equals)

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"', false, false)
local string = #S('\'"') * lexer.last_char_includes('=') * token(lexer.STRING, sq_str + dq_str)
lex:add_rule('string', string)

-- Numbers.
local number = token(lexer.NUMBER, lexer.dec_num * P('%')^-1)
lex:add_rule('number', #lexer.digit * lexer.last_char_includes('=') * number * in_tag)

-- Entities.
lex:add_rule('entity', token('entity', '&' * word_match('lt gt amp apos quot') * ';'))
lex:add_style('entity', lexer.styles.operator)

-- Fold points.
local function disambiguate_lt(text, pos, line, s) return not line:find('^</', s) and 1 or -1 end
lex:add_fold_point('element', '<', disambiguate_lt)
lex:add_fold_point('element', '/>', -1)
lex:add_fold_point(lexer.COMMENT, '<!--', '-->')

-- Finally, add JavaScript and VBScript as embedded languages

-- Tags that start embedded languages.
local embed_start_tag = element * (ws^1 * attribute * ws^0 * equals * ws^0 * string)^0 * ws^0 *
  tag_close
local embed_end_tag = element * tag_close

-- Embedded JavaScript.
local js = lexer.load('javascript')
local js_start_rule = #(P('<script') * (P(function(input, index)
  if input:find('^%s+language%s*=%s*(["\'])[jJ][ava]*[sS]cript%1', index) then return index end
end) + '>')) * embed_start_tag -- <script language="javascript">
local js_end_rule = #('</script' * ws^0 * '>') * embed_end_tag -- </script>
lex:embed(js, js_start_rule, js_end_rule)

-- Embedded VBScript.
local vbs = lexer.load('vb', 'vbscript')
local vbs_start_rule = #(P('<script') * (P(function(input, index)
  if input:find('^%s+language%s*=%s*(["\'])[vV][bB][sS]cript%1', index) then return index end
end) + '>')) * embed_start_tag -- <script language="vbscript">
local vbs_end_rule = #('</script' * ws^0 * '>') * embed_end_tag -- </script>
lex:embed(vbs, vbs_start_rule, vbs_end_rule)

return lex
