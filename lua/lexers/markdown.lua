-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Markdown LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('markdown')

-- Block elements.
local function h(n) return token('h' .. n, lexer.to_eol(lexer.starts_line(string.rep('#', n)))) end
lex:add_rule('header', h(6) + h(5) + h(4) + h(3) + h(2) + h(1))
local font_size = tonumber(lexer.property_expanded['style.default']:match('size:(%d+)')) or 10
local function add_header_style(n)
  lex:add_style('h' .. n, {fore = lexer.colors.red, size = (font_size + (6 - n))})
end
for i = 1, 6 do add_header_style(i) end

lex:add_rule('blockquote',
  token(lexer.STRING, lpeg.Cmt(lexer.starts_line(S(' \t')^0 * '>'), function(input, index)
    local _, e = input:find('\n[ \t]*\r?\n', index)
    return (e or #input) + 1
  end)))

lex:add_rule('list', token('list',
  lexer.starts_line(S(' \t')^0 * (S('*+-') + lexer.digit^1 * '.')) * S(' \t')))
lex:add_style('list', lexer.styles.constant)

local hspace = S('\t\v\f\r ')
local blank_line = '\n' * hspace^0 * ('\n' + P(-1))

local code_line = lexer.to_eol(lexer.starts_line(P(' ')^4 + '\t') * -P('<')) * lexer.newline^-1
local code_block = lexer.range(lexer.starts_line('```'), '\n```' * hspace^0 * ('\n' + P(-1)))
local code_inline = lpeg.Cmt(lpeg.C(P('`')^1), function(input, index, bt)
  -- `foo`, ``foo``, ``foo`bar``, `foo``bar` are all allowed.
  local _, e = input:find('[^`]' .. bt .. '%f[^`]', index)
  return (e or #input) + 1
end)
lex:add_rule('block_code', token('code', code_line + code_block + code_inline))
lex:add_style('code', lexer.styles.embedded .. {eolfilled = true})

lex:add_rule('hr',
  token('hr', lpeg.Cmt(lexer.starts_line(S(' \t')^0 * lpeg.C(S('*-_'))), function(input, index, c)
    local line = input:match('[^\r\n]*', index):gsub('[ \t]', '')
    if line:find('[^' .. c .. ']') or #line < 2 then return nil end
    return (select(2, input:find('\r?\n', index)) or #input) + 1
  end)))
lex:add_style('hr', {back = lexer.colors.black, eolfilled = true})

-- Whitespace.
local ws = token(lexer.WHITESPACE, S(' \t')^1 + S('\v\r\n')^1)
lex:add_rule('whitespace', ws)

-- Span elements.
lex:add_rule('escape', token(lexer.DEFAULT, P('\\') * 1))

local ref_link_label = token('link_label', lexer.range('[', ']', true) * ':')
local ref_link_url = token('link_url', (lexer.any - lexer.space)^1)
local ref_link_title = token(lexer.STRING, lexer.range('"', true, false) +
  lexer.range("'", true, false) + lexer.range('(', ')', true))
lex:add_rule('link_label', ref_link_label * ws * ref_link_url * (ws * ref_link_title)^-1)
lex:add_style('link_label', lexer.styles.label)
lex:add_style('link_url', {underlined = true})

local link_label = P('!')^-1 * lexer.range('[', ']', true)
local link_target =
  '(' * (lexer.any - S(') \t'))^0 * (S(' \t')^1 * lexer.range('"', false, false))^-1 * ')'
local link_ref = S(' \t')^0 * lexer.range('[', ']', true)
local link_url = 'http' * P('s')^-1 * '://' * (lexer.any - lexer.space)^1 +
  ('<' * lexer.alpha^2 * ':' * (lexer.any - lexer.space - '>')^1 * '>')
lex:add_rule('link', token('link', link_label * (link_target + link_ref) + link_url))
lex:add_style('link', {underlined = true})

local punct_space = lexer.punct + lexer.space

-- Handles flanking delimiters as described in
-- https://github.github.com/gfm/#emphasis-and-strong-emphasis in the cases where simple
-- delimited ranges are not sufficient.
local function flanked_range(s, not_inword)
  local fl_char = lexer.any - s - lexer.space
  local left_fl = lpeg.B(punct_space - s) * s * #fl_char + s * #(fl_char - lexer.punct)
  local right_fl = lpeg.B(lexer.punct) * s * #(punct_space - s) + lpeg.B(fl_char) * s
  return left_fl * (lexer.any - blank_line - (not_inword and s * #punct_space or s))^0 * right_fl
end

local asterisk_strong = flanked_range('**')
local underscore_strong = (lpeg.B(punct_space) + #lexer.starts_line('_')) *
  flanked_range('__', true) * #(punct_space + -1)
lex:add_rule('strong', token('strong', asterisk_strong + underscore_strong))
lex:add_style('strong', {bold = true})

local asterisk_em = flanked_range('*')
local underscore_em = (lpeg.B(punct_space) + #lexer.starts_line('_')) * flanked_range('_', true) *
  #(punct_space + -1)
lex:add_rule('em', token('em', asterisk_em + underscore_em))
lex:add_style('em', {italics = true})

-- Embedded HTML.
local html = lexer.load('html')
local start_rule = lexer.starts_line(P(' ')^-3) * #P('<') * html:get_rule('element') -- P(' ')^4 starts code_line
local end_rule = token(lexer.DEFAULT, blank_line) -- TODO: lexer.WHITESPACE errors
lex:embed(html, start_rule, end_rule)

return lex
