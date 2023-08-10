-- Copyright 2006-2024 Mitchell. See LICENSE.
-- reStructuredText LPeg lexer.

local lexer = require('lexer')
local token, word_match, starts_line = lexer.token, lexer.word_match, lexer.starts_line
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rest')

-- Literal block.
local block = '::' * (lexer.newline + -1) * function(input, index)
  local rest = input:sub(index)
  local level, quote = #rest:match('^([ \t]*)')
  for pos, indent, line in rest:gmatch('()[ \t]*()([^\r\n]+)') do
    local no_indent = (indent - pos < level and line ~= ' ' or level == 0)
    local quoted = no_indent and line:find(quote or '^%s*%W')
    if quoted and not quote then quote = '^%s*%' .. line:match('^%s*(%W)') end
    if no_indent and not quoted and pos > 1 then return index + pos - 1 end
  end
  return #input + 1
end
lex:add_rule('literal_block', token('literal_block', block))
lex:add_style('literal_block', lexer.styles.embedded .. {eolfilled = true})

-- Lists.
local option_word = lexer.alnum * (lexer.alnum + '-')^0
local option = S('-/') * option_word * (' ' * option_word)^-1 +
  ('--' * option_word * ('=' * option_word)^-1)
local option_list = option * (',' * lexer.space^1 * option)^-1
local bullet_list = S('*+-') -- TODO: '•‣⁃', as lpeg does not support UTF-8
local enum_list = P('(')^-1 * (lexer.digit^1 + S('ivxlcmIVXLCM')^1 + lexer.alnum + '#') * S('.)')
local field_list = ':' * (lexer.any - ':')^1 * P(':')^-1
lex:add_rule('list', #(lexer.space^0 * (S('*+-:/') + enum_list)) *
  starts_line(token(lexer.LIST,
    lexer.space^0 * (option_list + bullet_list + enum_list + field_list) * lexer.space)))

local any_indent = S(' \t')^0
local word = lexer.alpha * (lexer.alnum + S('-.+'))^0
local prefix = any_indent * '.. '

-- Explicit markup blocks.
local footnote_label = '[' * (lexer.digit^1 + '#' * word^-1 + '*') * ']'
local footnote = token('footnote_block', prefix * footnote_label * lexer.space)
local citation_label = '[' * word * ']'
local citation = token('citation_block', prefix * citation_label * lexer.space)
local link = token('link_block', prefix * '_' *
  (lexer.range('`') + (P('\\') * 1 + lexer.nonnewline - ':')^1) * ':' * lexer.space)
lex:add_rule('markup_block', #prefix * starts_line(footnote + citation + link))
lex:add_style('footnote_block', lexer.styles.label)
lex:add_style('citation_block', lexer.styles.label)
lex:add_style('link_block', lexer.styles.label)

-- Sphinx code block.
local indented_block = function(input, index)
  local rest = input:sub(index)
  local level = #rest:match('^([ \t]*)')
  for pos, indent, line in rest:gmatch('()[ \t]*()([^\r\n]+)') do
    if indent - pos < level and line ~= ' ' or level == 0 and pos > 1 then return index + pos - 1 end
  end
  return #input + 1
end
local code_block =
  prefix * 'code-block::' * S(' \t')^1 * lexer.nonnewline^0 * (lexer.newline + -1) * indented_block
lex:add_rule('code_block', #prefix * token('code_block', starts_line(code_block)))
lex:add_style('code_block', lexer.styles.embedded .. {eolfilled = true})

-- Directives.
local known_directive = token('directive', prefix * word_match{
  -- Admonitions
  'attention', 'caution', 'danger', 'error', 'hint', 'important', 'note', 'tip', 'warning',
  'admonition',
  -- Images
  'image', 'figure',
  -- Body elements
  'topic', 'sidebar', 'line-block', 'parsed-literal', 'code', 'math', 'rubric', 'epigraph',
  'highlights', 'pull-quote', 'compound', 'container',
  -- Table
  'table', 'csv-table', 'list-table',
  -- Document parts
  'contents', 'sectnum', 'section-autonumbering', 'header', 'footer',
  -- References
  'target-notes', 'footnotes', 'citations',
  -- HTML-specific
  'meta',
  -- Directives for substitution definitions
  'replace', 'unicode', 'date',
  -- Miscellaneous
  'include', 'raw', 'class', 'role', 'default-role', 'title', 'restructuredtext-test-directive'
} * '::' * lexer.space)
local sphinx_directive = token('sphinx_directive', prefix * word_match{
  -- The TOC tree.
  'toctree',
  -- Paragraph-level markup.
  'note', 'warning', 'versionadded', 'versionchanged', 'deprecated', 'seealso', 'rubric',
  'centered', 'hlist', 'glossary', 'productionlist',
  -- Showing code examples.
  'highlight', 'literalinclude',
  -- Miscellaneous
  'sectionauthor', 'index', 'only', 'tabularcolumns'
} * '::' * lexer.space)
local unknown_directive = token('unknown_directive', prefix * word * '::' * lexer.space)
lex:add_rule('directive',
  #prefix * starts_line(known_directive + sphinx_directive + unknown_directive))
lex:add_style('directive', lexer.styles.keyword)
lex:add_style('sphinx_directive', lexer.styles.keyword .. {bold = true})
lex:add_style('unknown_directive', lexer.styles.keyword .. {italics = true})

-- Substitution definitions.
lex:add_rule('substitution', #prefix * token('substitution', starts_line(prefix * lexer.range('|') *
  lexer.space^1 * word * '::' * lexer.space)))
lex:add_style('substitution', lexer.styles.variable)

-- Comments.
local line_comment = lexer.to_eol(prefix)
local bprefix = any_indent * '..'
local block_comment = bprefix * lexer.newline * indented_block
lex:add_rule('comment', #bprefix * token(lexer.COMMENT, starts_line(line_comment + block_comment)))

-- Section titles (2 or more characters).
local adornment_chars = lpeg.C(S('!"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~'))
local adornment = lpeg.C(adornment_chars^2 * any_indent) * (lexer.newline + -1)
local overline = lpeg.Cmt(starts_line(adornment), function(input, index, adm, c)
  if not adm:find('^%' .. c .. '+%s*$') then return nil end
  local rest = input:sub(index)
  local lines = 1
  for line, e in rest:gmatch('([^\r\n]+)()') do
    if lines > 1 and line:match('^(%' .. c .. '+)%s*$') == adm then return index + e - 1 end
    if lines > 3 or #line > #adm then return nil end
    lines = lines + 1
  end
  return #input + 1
end)
local underline = lpeg.Cmt(starts_line(adornment), function(_, index, adm, c)
  local pos = adm:match('^%' .. c .. '+%s*()$')
  return pos and index - #adm + pos - 1 or nil
end)
-- Token needs to be a predefined one in order for folder to work.
lex:add_rule('title', token(lexer.HEADING, overline + underline))

-- Line block.
lex:add_rule('line_block_char', token(lexer.OPERATOR, starts_line(any_indent * '|')))

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, S(' \t')^1 + lexer.newline^1))

-- Inline markup.
local strong = token(lexer.BOLD, lexer.range('**'))
local em = token(lexer.ITALIC, lexer.range('*'))
local inline_literal = token('inline_literal', lexer.range('``'))
local postfix_link = (word + lexer.range('`')) * '_' * P('_')^-1
local prefix_link = '_' * lexer.range('`')
local link_ref = token(lexer.LINK, postfix_link + prefix_link)
local role = token('role', ':' * word * ':' * (word * ':')^-1)
local interpreted = role^-1 * token('interpreted', lexer.range('`')) * role^-1
local footnote_ref = token(lexer.REFERENCE, footnote_label * '_')
local citation_ref = token(lexer.REFERENCE, citation_label * '_')
local substitution_ref = token('substitution', lexer.range('|', true) * ('_' * P('_')^-1)^-1)
local link = token(lexer.LINK,
  lexer.alpha * (lexer.alnum + S('-.'))^1 * ':' * (lexer.alnum + S('/.+-%@'))^1)
lex:add_rule('inline_markup',
  (strong + em + inline_literal + link_ref + interpreted + footnote_ref + citation_ref +
    substitution_ref + link) * -lexer.alnum)
lex:add_style('inline_literal', lexer.styles.embedded)
lex:add_style('role', lexer.styles.class)
lex:add_style('interpreted', lexer.styles.string)

-- Other.
lex:add_rule('non_space', token(lexer.DEFAULT, lexer.alnum * (lexer.any - lexer.space)^0))
lex:add_rule('escape', token(lexer.DEFAULT, '\\' * lexer.any))

-- Section-based folding.
local sphinx_levels = {
  ['#'] = 0, ['*'] = 1, ['='] = 2, ['-'] = 3, ['^'] = 4, ['"'] = 5
}

function lex:fold(text, start_pos, start_line, start_level)
  local folds, line_starts = {}, {}
  for pos in (text .. '\n'):gmatch('().-\r?\n') do line_starts[#line_starts + 1] = pos end
  local style_at, CONSTANT, level = lexer.style_at, lexer.CONSTANT, start_level
  local sphinx = lexer.property_int['fold.scintillua.rest.by.sphinx.convention'] > 0
  local FOLD_BASE = lexer.FOLD_BASE
  local FOLD_HEADER, FOLD_BLANK = lexer.FOLD_HEADER, lexer.FOLD_BLANK
  for i = 1, #line_starts do
    local pos, next_pos = line_starts[i], line_starts[i + 1]
    local c = text:sub(pos, pos)
    local line_num = start_line + i - 1
    folds[line_num] = level
    if style_at[start_pos + pos - 1] == CONSTANT and c:find('^[^%w%s]') then
      local sphinx_level = FOLD_BASE + (sphinx_levels[c] or #sphinx_levels)
      level = not sphinx and level - 1 or sphinx_level
      if level < FOLD_BASE then level = FOLD_BASE end
      folds[line_num - 1], folds[line_num] = level, level + FOLD_HEADER
      level = (not sphinx and level or sphinx_level) + 1
    elseif c == '\r' or c == '\n' then
      folds[line_num] = level + FOLD_BLANK
    end
  end
  return folds
end

-- lexer.property['fold.by.sphinx.convention'] = '0'

--[[ Embedded languages.
local bash = lexer.load('bash')
local bash_indent_level
local start_rule =
  #(prefix * 'code-block' * '::' * lexer.space^1 * 'bash' * (lexer.newline + -1)) *
  sphinx_directive * token('bash_begin', P(function(input, index)
    bash_indent_level = #input:match('^([ \t]*)', index)
    return index
  end))]]

lexer.property['scintillua.comment'] = '.. '

return lex
