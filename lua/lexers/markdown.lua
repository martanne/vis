-- Copyright 2006-2024 Mitchell. See LICENSE.
-- Markdown LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(..., {no_user_word_lists = true})

-- Distinguish between horizontal and vertical space so html start rule has a chance to match.
lex:modify_rule('whitespace', lex:tag(lexer.WHITESPACE, S(' \t')^1 + S('\r\n')^1))

-- Block elements.
local function h(n)
  return lex:tag(string.format('%s.h%s', lexer.HEADING, n),
    lexer.to_eol(lexer.starts_line(string.rep('#', n))))
end
lex:add_rule('header', h(6) + h(5) + h(4) + h(3) + h(2) + h(1))

lex:add_rule('hr',
  lex:tag('hr', lpeg.Cmt(lexer.starts_line(lpeg.C(S('*-_')), true), function(input, index, c)
    local line = input:match('[^\r\n]*', index):gsub('[ \t]', '')
    if line:find('[^' .. c .. ']') or #line < 2 then return nil end
    return (select(2, input:find('\r?\n', index)) or #input) + 1 -- include \n for eolfilled styles
  end)))

lex:add_rule('list', lex:tag(lexer.LIST,
  lexer.starts_line(lexer.digit^1 * '.' + S('*+-'), true) * S(' \t')))

local hspace = lexer.space - '\n'
local blank_line = '\n' * hspace^0 * ('\n' + P(-1))

local code_line = lexer.starts_line((B('    ') + B('\t')) * lexer.to_eol(), true)
local code_block =
  lexer.range(lexer.starts_line('```', true), '\n```' * hspace^0 * ('\n' + P(-1))) +
    lexer.range(lexer.starts_line('~~~', true), '\n~~~' * hspace^0 * ('\n' + P(-1)))
local code_inline = lpeg.Cmt(lpeg.C(P('`')^1), function(input, index, bt)
  -- `foo`, ``foo``, ``foo`bar``, `foo``bar` are all allowed.
  local _, e = input:find('[^`]' .. bt .. '%f[^`]', index)
  return (e or #input) + 1
end)
lex:add_rule('block_code', lex:tag(lexer.CODE, code_line + code_block + code_inline))

lex:add_rule('blockquote',
  lex:tag(lexer.STRING, lpeg.Cmt(lexer.starts_line('>', true), function(input, index)
    local _, e = input:find('\n[ \t]*\r?\n', index) -- the next blank line (possibly with indentation)
    return (e or #input) + 1
  end)))

-- Span elements.
lex:add_rule('escape', lex:tag(lexer.DEFAULT, P('\\') * 1))

local link_text = lexer.range('[', ']', true)
local link_target =
  '(' * (lexer.any - S(') \t'))^0 * (S(' \t')^1 * lexer.range('"', false, false))^-1 * ')'
local link_url = 'http' * P('s')^-1 * '://' * (lexer.any - lexer.space)^1 +
  ('<' * lexer.alpha^2 * ':' * (lexer.any - lexer.space - '>')^1 * '>')
lex:add_rule('link', lex:tag(lexer.LINK, P('!')^-1 * link_text * link_target + link_url))

local link_ref = lex:tag(lexer.REFERENCE, link_text * S(' \t')^0 * lexer.range('[', ']', true))
local ref_link_label = lex:tag(lexer.REFERENCE, lexer.range('[', ']', true) * ':')
local ws = lex:get_rule('whitespace')
local ref_link_url = lex:tag(lexer.LINK, (lexer.any - lexer.space)^1)
local ref_link_title = lex:tag(lexer.STRING, lexer.range('"', true, false) +
  lexer.range("'", true, false) + lexer.range('(', ')', true))
lex:add_rule('link_ref', link_ref + ref_link_label * ws * ref_link_url * (ws * ref_link_title)^-1)

local punct_space = lexer.punct + lexer.space

-- Handles flanking delimiters as described in
-- https://github.github.com/gfm/#emphasis-and-strong-emphasis in the cases where simple
-- delimited ranges are not sufficient.
local function flanked_range(s, not_inword)
  local fl_char = lexer.any - s - lexer.space
  local left_fl = B(punct_space - s) * s * #fl_char + s * #(fl_char - lexer.punct)
  local right_fl = B(lexer.punct) * s * #(punct_space - s) + B(fl_char) * s
  return left_fl * (lexer.any - blank_line - (not_inword and s * #punct_space or s))^0 * right_fl
end

local asterisk_strong = flanked_range('**')
local underscore_strong = (B(punct_space) + #lexer.starts_line('_')) * flanked_range('__', true) *
  #(punct_space + -1)
lex:add_rule('strong', lex:tag(lexer.BOLD, asterisk_strong + underscore_strong))

local asterisk_em = flanked_range('*')
local underscore_em = (B(punct_space) + #lexer.starts_line('_')) * flanked_range('_', true) *
  #(punct_space + -1)
lex:add_rule('em', lex:tag(lexer.ITALIC, asterisk_em + underscore_em))

-- Embedded HTML.
local html = lexer.load('html')
local start_rule = lexer.starts_line(P(' ')^-3) * #P('<') * html:get_rule('tag') -- P(' ')^4 starts code_line
local end_rule = #blank_line * ws
lex:embed(html, start_rule, end_rule)

return lex
