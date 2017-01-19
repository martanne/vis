-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Markdown LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'markdown'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Block elements.
local header = token('h6', l.starts_line('######') * l.nonnewline^0) +
               token('h5', l.starts_line('#####') * l.nonnewline^0) +
               token('h4', l.starts_line('####') * l.nonnewline^0) +
               token('h3', l.starts_line('###') * l.nonnewline^0) +
               token('h2', l.starts_line('##') * l.nonnewline^0) +
               token('h1', l.starts_line('#') * l.nonnewline^0)

local blockquote = token(l.STRING,
                         lpeg.Cmt(l.starts_line(S(' \t')^0 * '>'),
                                  function(input, index)
                                    local _, e = input:find('\n[ \t]*\r?\n',
                                                            index)
                                    return (e or #input) + 1
                                  end))

local blockcode = token('code', l.starts_line(P(' ')^4 + P('\t')) * -P('<') *
                                l.nonnewline^0)

local hr = token('hr', lpeg.Cmt(l.starts_line(S(' \t')^0 * lpeg.C(S('*-_'))),
                                function(input, index, c)
                                  local line = input:match('[^\n]*', index)
                                  line = line:gsub('[ \t]', '')
                                  if line:find('[^'..c..']') or #line < 2 then
                                    return nil
                                  end
                                  return (input:find('\n', index) or #input) + 1
                                end))

-- Span elements.
local dq_str = token(l.STRING, l.delimited_range('"', false, true))
local sq_str = token(l.STRING, l.delimited_range("'", false, true))
local paren_str = token(l.STRING, l.delimited_range('()'))
local link = token('link', P('!')^-1 * l.delimited_range('[]') *
                           (P('(') * (l.any - S(') \t'))^0 *
                            (S(' \t')^1 *
                             l.delimited_range('"', false, true))^-1 * ')' +
                            S(' \t')^0 * l.delimited_range('[]')) +
                           P('http://') * (l.any - l.space)^1)
local link_label = token('link_label', l.delimited_range('[]') * ':') * ws *
                   token('link_url', (l.any - l.space)^1) *
                   (ws * (dq_str + sq_str + paren_str))^-1

local strong = token('strong', (P('**') * (l.any - '**')^0 * P('**')^-1) +
                               (P('__') * (l.any - '__')^0 * P('__')^-1))
local em = token('em',
                 l.delimited_range('*', true) + l.delimited_range('_', true))
local code = token('code', (P('``') * (l.any - '``')^0 * P('``')^-1) +
                           l.delimited_range('`', true, true))

local escape = token(l.DEFAULT, P('\\') * 1)

local list = token('list',
                   l.starts_line(S(' \t')^0 * (S('*+-') + R('09')^1 * '.')) *
                   S(' \t'))

M._rules = {
  {'header', header},
  {'blockquote', blockquote},
  {'blockcode', blockcode},
  {'hr', hr},
  {'list', list},
  {'whitespace', ws},
  {'link_label', link_label},
  {'escape', escape},
  {'link', link},
  {'strong', strong},
  {'em', em},
  {'code', code},
}

local font_size = 10
local hstyle = 'fore:red'
M._tokenstyles = {
  h6 = hstyle,
  h5 = hstyle..',size:'..(font_size + 1),
  h4 = hstyle..',size:'..(font_size + 2),
  h3 = hstyle..',size:'..(font_size + 3),
  h2 = hstyle..',size:'..(font_size + 4),
  h1 = hstyle..',size:'..(font_size + 5),
  code = l.STYLE_EMBEDDED..',eolfilled',
  hr = l.STYLE_DEFAULT..',bold',
  link = 'underlined',
  link_url = 'underlined',
  link_label = l.STYLE_LABEL,
  strong = 'bold',
  em = 'italics',
  list = l.STYLE_CONSTANT,
}

-- Embedded HTML.
local html = l.load('html')
local start_rule = token('tag', l.starts_line(S(' \t')^0 * '<'))
local end_rule = token(l.DEFAULT, P('\n')) -- TODO: l.WHITESPACE causes errors
l.embed_lexer(M, html, start_rule, end_rule)

return M
