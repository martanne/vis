-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- reStructuredText LPeg lexer.

local l = require('lexer')
local token, word_match, starts_line = l.token, l.word_match, l.starts_line
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'rest'}

-- Whitespace.
local ws = token(l.WHITESPACE, S(' \t')^1 + l.newline^1)
local any_indent = S(' \t')^0

-- Section titles (2 or more characters).
local adornment_chars = lpeg.C(S('!"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~'))
local adornment = lpeg.C(adornment_chars^2 * any_indent) * (l.newline + -1)
local overline = lpeg.Cmt(starts_line(adornment), function(input, index, adm, c)
  if not adm:find('^%'..c..'+%s*$') then return nil end
  local rest = input:sub(index)
  local lines = 1
  for line, e in rest:gmatch('([^\r\n]+)()') do
    if lines > 1 and line:match('^(%'..c..'+)%s*$') == adm then
      return index + e - 1
    end
    if lines > 3 or #line > #adm then return nil end
    lines = lines + 1
  end
  return #input + 1
end)
local underline = lpeg.Cmt(starts_line(adornment), function(_, index, adm, c)
  local pos = adm:match('^%'..c..'+%s*()$')
  return pos and index - #adm + pos - 1 or nil
end)
-- Token needs to be a predefined one in order for folder to work.
local title = token(l.CONSTANT, overline + underline)

-- Lists.
local bullet_list = S('*+-') -- TODO: '•‣⁃', as lpeg does not support UTF-8
local enum_list = P('(')^-1 *
                  (l.digit^1 + S('ivxlcmIVXLCM')^1 + l.alnum + '#') * S('.)')
local field_list = ':' * (l.any - ':')^1 * P(':')^-1
local option_word = l.alnum * (l.alnum + '-')^0
local option = S('-/') * option_word * (' ' * option_word)^-1 +
               '--' * option_word * ('=' * option_word)^-1
local option_list = option * (',' * l.space^1 * option)^-1
local list = #(l.space^0 * (S('*+-:/') + enum_list)) *
             starts_line(token('list', l.space^0 * (option_list + bullet_list +
                                                    enum_list + field_list) *
                                                   l.space))

-- Literal block.
local block = P('::') * (l.newline + -1) * function(input, index)
  local rest = input:sub(index)
  local level, quote = #rest:match('^([ \t]*)')
  for pos, indent, line in rest:gmatch('()[ \t]*()([^\r\n]+)') do
    local no_indent = (indent - pos < level and line ~= ' ' or level == 0)
    local quoted = no_indent and line:find(quote or '^%s*%W')
    if quoted and not quote then quote = '^%s*%'..line:match('^%s*(%W)') end
    if no_indent and not quoted and pos > 1 then return index + pos - 1 end
  end
  return #input + 1
end
local literal_block = token('literal_block', block)

-- Line block.
local line_block_char = token(l.OPERATOR, starts_line(any_indent * '|'))

local word = l.alpha * (l.alnum + S('-.+'))^0

-- Explicit markup blocks.
local prefix = any_indent * '.. '
local footnote_label = '[' * (l.digit^1 + '#' * word^-1 + '*') * ']'
local footnote = token('footnote_block', prefix * footnote_label * l.space)
local citation_label = '[' * word * ']'
local citation = token('citation_block', prefix * citation_label * l.space)
local link = token('link_block', prefix * '_' *
                                 (l.delimited_range('`') + (P('\\') * 1 +
                                  l.nonnewline - ':')^1) * ':' * l.space)
local markup_block = #prefix * starts_line(footnote + citation + link)

-- Directives.
local directive_type = word_match({
  -- Admonitions
  'attention', 'caution', 'danger', 'error', 'hint', 'important', 'note', 'tip',
  'warning', 'admonition',
  -- Images
  'image', 'figure',
  -- Body elements
  'topic', 'sidebar', 'line-block', 'parsed-literal', 'code', 'math', 'rubric',
  'epigraph', 'highlights', 'pull-quote', 'compound', 'container',
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
  'include', 'raw', 'class', 'role', 'default-role', 'title',
  'restructuredtext-test-directive',
}, '-')
local known_directive = token('directive',
                              prefix * directive_type * '::' * l.space)
local sphinx_directive_type = word_match({
  -- The TOC tree.
  'toctree',
  -- Paragraph-level markup.
  'note', 'warning', 'versionadded', 'versionchanged', 'deprecated', 'seealso',
  'rubric', 'centered', 'hlist', 'glossary', 'productionlist',
  -- Showing code examples.
  'highlight', 'literalinclude',
  -- Miscellaneous
  'sectionauthor', 'index', 'only', 'tabularcolumns'
}, '-')
local sphinx_directive = token('sphinx_directive',
                               prefix * sphinx_directive_type * '::' * l.space)
local unknown_directive = token('unknown_directive',
                                prefix * word * '::' * l.space)
local directive = #prefix * starts_line(known_directive + sphinx_directive +
                                        unknown_directive)

-- Sphinx code block.
local indented_block = function(input, index)
  local rest = input:sub(index)
  local level = #rest:match('^([ \t]*)')
  for pos, indent, line in rest:gmatch('()[ \t]*()([^\r\n]+)') do
    if indent - pos < level and line ~= ' ' or level == 0 and pos > 1 then
      return index + pos - 1
    end
  end
  return #input + 1
end
local code_block = prefix * 'code-block::' * S(' \t')^1 * l.nonnewline^0 *
                   (l.newline + -1) * indented_block
local sphinx_block = #prefix * token('code_block', starts_line(code_block))

-- Substitution definitions.
local substitution = #prefix *
                     token('substitution',
                           starts_line(prefix * l.delimited_range('|') *
                                       l.space^1 * word * '::' * l.space))

-- Comments.
local line_comment = prefix * l.nonnewline^0
local bprefix = any_indent * '..'
local block_comment = bprefix * l.newline * indented_block
local comment = #bprefix *
                token(l.COMMENT, starts_line(line_comment + block_comment))

-- Inline markup.
local em = token('em', l.delimited_range('*'))
local strong = token('strong', ('**' * (l.any - '**')^0 * P('**')^-1))
local role = token('role', ':' * word * ':' * (word * ':')^-1)
local interpreted = role^-1 * token('interpreted', l.delimited_range('`')) *
                    role^-1
local inline_literal = token('inline_literal',
                             '``' * (l.any - '``')^0 * P('``')^-1)
local link_ref = token('link',
                       (word + l.delimited_range('`')) * '_' * P('_')^-1 +
                       '_' * l.delimited_range('`'))
local footnote_ref = token('footnote', footnote_label * '_')
local citation_ref = token('citation', citation_label * '_')
local substitution_ref = token('substitution', l.delimited_range('|', true) *
                                               ('_' * P('_')^-1)^-1)
local link = token('link', l.alpha * (l.alnum + S('-.'))^1 * ':' *
                           (l.alnum + S('/.+-%@'))^1)
local inline_markup = (strong + em + inline_literal + link_ref + interpreted +
                       footnote_ref + citation_ref + substitution_ref + link) *
                      -l.alnum

-- Other.
local non_space = token(l.DEFAULT, l.alnum * (l.any - l.space)^0)
local escape = token(l.DEFAULT, '\\' * l.any)

M._rules = {
  {'literal_block', literal_block},
  {'list', list},
  {'markup_block', markup_block},
  {'code_block', sphinx_block},
  {'directive', directive},
  {'substitution', substitution},
  {'comment', comment},
  {'title', title},
  {'line_block_char', line_block_char},
  {'whitespace', ws},
  {'inline_markup', inline_markup},
  {'non_space', non_space},
  {'escape', escape}
}

M._tokenstyles = {
  list = l.STYLE_TYPE,
  literal_block = l.STYLE_EMBEDDED..',eolfilled',
  footnote_block = l.STYLE_LABEL,
  citation_block = l.STYLE_LABEL,
  link_block = l.STYLE_LABEL,
  directive = l.STYLE_KEYWORD,
  sphinx_directive = l.STYLE_KEYWORD..',bold',
  unknown_directive = l.STYLE_KEYWORD..',italics',
  code_block = l.STYLE_EMBEDDED..',eolfilled',
  substitution = l.STYLE_VARIABLE,
  strong = 'bold',
  em = 'italics',
  role = l.STYLE_CLASS,
  interpreted = l.STYLE_STRING,
  inline_literal = l.STYLE_EMBEDDED,
  link = 'underlined',
  footnote = 'underlined',
  citation = 'underlined',
}

local sphinx_levels = {
  ['#'] = 0, ['*'] = 1, ['='] = 2, ['-'] = 3, ['^'] = 4, ['"'] = 5
}

-- Section-based folding.
M._fold = function(text, start_pos, start_line, start_level)
  local folds, line_starts = {}, {}
  for pos in (text..'\n'):gmatch('().-\r?\n') do
    line_starts[#line_starts + 1] = pos
  end
  local style_at, CONSTANT, level = l.style_at, l.CONSTANT, start_level
  local sphinx = l.property_int['fold.by.sphinx.convention'] > 0
  local FOLD_BASE = l.FOLD_BASE
  local FOLD_HEADER, FOLD_BLANK = l.FOLD_HEADER, l.FOLD_BLANK
  for i = 1, #line_starts do
    local pos, next_pos = line_starts[i], line_starts[i + 1]
    local c = text:sub(pos, pos)
    local line_num = start_line + i - 1
    folds[line_num] = level
    if style_at[start_pos + pos] == CONSTANT and c:find('^[^%w%s]') then
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

l.property['fold.by.sphinx.convention'] = '0'

--[[ Embedded languages.
local bash = l.load('bash')
local bash_indent_level
local start_rule = #(prefix * 'code-block' * '::' * l.space^1 * 'bash' *
                     (l.newline + -1)) * sphinx_directive *
                    token('bash_begin', P(function(input, index)
                      bash_indent_level = #input:match('^([ \t]*)', index)
                      return index
                    end))]]

return M
