-- Copyright 2019-2022 Julien L. See LICENSE.
-- txt2tags LPeg lexer.
-- (developed and tested with Txt2tags Markup Rules
-- [https://txt2tags.org/doc/english/rules.t2t])
-- Contributed by Julien L.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S
local nonspace = lexer.any - lexer.space

local lex = lexer.new('txt2tags')

-- Whitespace.
local ws = token(lexer.WHITESPACE, (lexer.space - lexer.newline)^1)

-- Titles
local alphanumeric = lexer.alnum + S('_-')
local header_label = token('header_label_start', '[') * token('header_label', alphanumeric^1) *
  token('header_label_end', ']')
local function h(level)
  local equal = string.rep('=', level) * (lexer.nonnewline - '=')^1 * string.rep('=', level)
  local plus = string.rep('+', level) * (lexer.nonnewline - '+')^1 * string.rep('+', level)
  return token('h' .. level, equal + plus) * header_label^-1
end
local header = h(5) + h(4) + h(3) + h(2) + h(1)

-- Comments.
local line_comment = lexer.to_eol(lexer.starts_line('%'))
local block_comment = lexer.range(lexer.starts_line('%%%'))
local comment = token(lexer.COMMENT, block_comment + line_comment)

-- Inline.
local function span(name, delimiter)
  return token(name, (delimiter * nonspace * delimiter * S(delimiter)^0) +
    (delimiter * nonspace * (lexer.nonnewline - nonspace * delimiter)^0 * nonspace * delimiter *
      S(delimiter)^0))
end
local bold = span('bold', '**')
local italic = span('italic', '//')
local underline = span('underline', '__')
local strike = span('strike', '--')
local mono = span('mono', '``')
local raw = span('raw', '""')
local tagged = span('tagged', "''")
local inline = bold + italic + underline + strike + mono + raw + tagged

-- Link.
local email = token('email',
  (nonspace - '@')^1 * '@' * (nonspace - '.')^1 * ('.' * (nonspace - S('.?'))^1)^1 *
    ('?' * nonspace^1)^-1)
local host = token('host',
  word_match('www ftp', true) * (nonspace - '.')^0 * '.' * (nonspace - '.')^1 * '.' *
    (nonspace - S(',.'))^1)
local url = token('url',
  (nonspace - '://')^1 * '://' * (nonspace - ',' - '.')^1 * ('.' * (nonspace - S(',./?#'))^1)^1 *
    ('/' * (nonspace - S('./?#'))^0 * ('.' * (nonspace - S(',.?#'))^1)^0)^0 *
    ('?' * (nonspace - '#')^1)^-1 * ('#' * nonspace^0)^-1)
local label_with_address = token('label_start', '[') * lexer.space^0 *
  token('address_label', ((nonspace - ']')^1 * lexer.space^1)^1) *
  token('address', (nonspace - ']')^1) * token('label_end', ']')
local link = label_with_address + url + host + email

-- Line.
local line = token('line', S('-=_')^20)

-- Image.
local image_only = token('image_start', '[') * token('image', (nonspace - ']')^1) *
  token('image_end', ']')
local image_link = token('image_link_start', '[') * image_only *
  token('image_link_sep', lexer.space^1) * token('image_link', (nonspace - ']')^1) *
  token('image_link_end', ']')
local image = image_link + image_only

-- Macro.
local macro = token('macro', '%%' * (nonspace - '(')^1 * lexer.range('(', ')', true)^-1)

-- Verbatim.
local verbatim_line = lexer.to_eol(lexer.starts_line('```') * S(' \t'))
local verbatim_block = lexer.range(lexer.starts_line('```'))
local verbatim_area = token('verbatim_area', verbatim_block + verbatim_line)

-- Raw.
local raw_line = lexer.to_eol(lexer.starts_line('"""') * S(' \t'))
local raw_block = lexer.range(lexer.starts_line('"""'))
local raw_area = token('raw_area', raw_block + raw_line)

-- Tagged.
local tagged_line = lexer.to_eol(lexer.starts_line('\'\'\'') * S(' \t'))
local tagged_block = lexer.range(lexer.starts_line('\'\'\''))
local tagged_area = token('tagged_area', tagged_block + tagged_line)

-- Table.
local table_sep = token('table_sep', '|')
local cell_content = inline + link + image + macro + token('cell_content', lexer.nonnewline - ' |')
local header_cell_content = token('header_cell_content', lexer.nonnewline - ' |')
local field_sep = ' ' * table_sep^1 * ' '
local table_row_end = P(' ')^0 * table_sep^0
local table_row = lexer.starts_line(P(' ')^0 * table_sep) * cell_content^0 *
  (field_sep * cell_content^0)^0 * table_row_end
local table_row_header =
  lexer.starts_line(P(' ')^0 * table_sep * table_sep) * header_cell_content^0 *
    (field_sep * header_cell_content^0)^0 * table_row_end
local table = table_row_header + table_row

lex:add_rule('table', table)
lex:add_rule('link', link)
lex:add_rule('line', line)
lex:add_rule('header', header)
lex:add_rule('comment', comment)
lex:add_rule('whitespace', ws)
lex:add_rule('image', image)
lex:add_rule('macro', macro)
lex:add_rule('inline', inline)
lex:add_rule('verbatim_area', verbatim_area)
lex:add_rule('raw_area', raw_area)
lex:add_rule('tagged_area', tagged_area)

lex:add_style('line', {bold = true})
local font_size = tonumber(lexer.property_expanded['style.default']:match('size:(%d+)')) or 10
for n = 5, 1, -1 do
  lex:add_style('h' .. n, {fore = lexer.colors.red, size = font_size + (6 - n)})
end
lex:add_style('header_label', lexer.styles.label)
lex:add_style('email', {underlined = true})
lex:add_style('host', {underlined = true})
lex:add_style('url', {underlined = true})
lex:add_style('address_label', lexer.styles.label)
lex:add_style('address', {underlined = true})
lex:add_style('image', {fore = lexer.colors.green})
lex:add_style('image_link', {underlined = true})
lex:add_style('macro', lexer.styles.preprocessor)
lex:add_style('bold', {bold = true})
lex:add_style('italic', {italics = true})
lex:add_style('underline', {underlined = true})
lex:add_style('strike', {italics = true}) -- a strike style is not available
lex:add_style('mono', {font = 'mono'})
lex:add_style('raw', {back = lexer.colors.grey})
lex:add_style('tagged', lexer.styles.embedded)
lex:add_style('verbatim_area', {font = 'mono'}) -- in consistency with mono
lex:add_style('raw_area', {back = lexer.colors.grey}) -- in consistency with raw
lex:add_style('tagged_area', lexer.styles.embedded) -- in consistency with tagged
lex:add_style('table_sep', {fore = lexer.colors.green})
lex:add_style('header_cell_content', {fore = lexer.colors.green})

return lex
