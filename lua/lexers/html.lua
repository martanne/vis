-- Copyright 2006-2022 Mitchell. See LICENSE.
-- HTML LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('html')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.range('<!--', '-->')))

-- Doctype.
lex:add_rule('doctype', token('doctype', lexer.range('<!' * word_match('doctype', true), '>')))
lex:add_style('doctype', lexer.styles.comment)

-- Elements.
local single_element = token('single_element', '<' * P('/')^-1 * word_match(
  {
    'area', 'base', 'br', 'col', 'command', 'embed', 'hr', 'img', 'input', 'keygen', 'link', 'meta',
    'param', 'source', 'track', 'wbr'
  }, true))
local paired_element = token('element', '<' * P('/')^-1 * word_match({
  'a', 'abbr', 'address', 'article', 'aside', 'audio', 'b', 'bdi', 'bdo', 'blockquote', 'body',
  'button', 'canvas', 'caption', 'cite', 'code', 'colgroup', 'content', 'data', 'datalist', 'dd',
  'decorator', 'del', 'details', 'dfn', 'div', 'dl', 'dt', 'element', 'em', 'fieldset',
  'figcaption', 'figure', 'footer', 'form', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head', 'header',
  'html', 'i', 'iframe', 'ins', 'kbd', 'label', 'legend', 'li', 'main', 'map', 'mark', 'menu',
  'menuitem', 'meter', 'nav', 'noscript', 'object', 'ol', 'optgroup', 'option', 'output', 'p',
  'pre', 'progress', 'q', 'rp', 'rt', 'ruby', 's', 'samp', 'script', 'section', 'select', 'shadow',
  'small', 'spacer', 'span', 'strong', 'style', 'sub', 'summary', 'sup', 'table', 'tbody', 'td',
  'template', 'textarea', 'tfoot', 'th', 'thead', 'time', 'title', 'tr', 'u', 'ul', 'var', 'video'
}, true))
local known_element = single_element + paired_element
local unknown_element = token('unknown_element', '<' * P('/')^-1 * (lexer.alnum + '-')^1)
local element = (known_element + unknown_element) * -P(':')
lex:add_rule('element', element)
lex:add_style('single_element', lexer.styles.keyword)
lex:add_style('element', lexer.styles.keyword)
lex:add_style('unknown_element', lexer.styles.keyword .. {italics = true})

-- Closing tags.
local tag_close = token('element', P('/')^-1 * '>')
lex:add_rule('tag_close', tag_close)

-- Attributes.
local known_attribute = token('attribute', word_match({
  'accept', 'accept-charset', 'accesskey', 'action', 'align', 'alt', 'async', 'autocomplete',
  'autofocus', 'autoplay', 'bgcolor', 'border', 'buffered', 'challenge', 'charset', 'checked',
  'cite', 'class', 'code', 'codebase', 'color', 'cols', 'colspan', 'content', 'contenteditable',
  'contextmenu', 'controls', 'coords', 'data', 'data-', 'datetime', 'default', 'defer', 'dir',
  'dirname', 'disabled', 'download', 'draggable', 'dropzone', 'enctype', 'for', 'form', 'headers',
  'height', 'hidden', 'high', 'href', 'hreflang', 'http-equiv', 'icon', 'id', 'ismap', 'itemprop',
  'keytype', 'kind', 'label', 'lang', 'language', 'list', 'loop', 'low', 'manifest', 'max',
  'maxlength', 'media', 'method', 'min', 'multiple', 'name', 'novalidate', 'open', 'optimum',
  'pattern', 'ping', 'placeholder', 'poster', 'preload', 'pubdate', 'radiogroup', 'readonly', 'rel',
  'required', 'reversed', 'role', 'rows', 'rowspan', 'sandbox', 'scope', 'scoped', 'seamless',
  'selected', 'shape', 'size', 'sizes', 'span', 'spellcheck', 'src', 'srcdoc', 'srclang', 'start',
  'step', 'style', 'summary', 'tabindppex', 'target', 'title', 'type', 'usemap', 'value', 'width',
  'wrap'
}, true) + ((P('data-') + 'aria-') * (lexer.alnum + '-')^1))
local unknown_attribute = token('unknown_attribute', (lexer.alnum + '-')^1)
local attribute = (known_attribute + unknown_attribute) * #(lexer.space^0 * '=')
lex:add_rule('attribute', attribute)
lex:add_style('attribute', lexer.styles.type)
lex:add_style('unknown_attribute', lexer.styles.type .. {italics = true})

-- Equals.
-- TODO: performance is terrible on large files.
local in_tag = P(function(input, index)
  local before = input:sub(1, index - 1)
  local s, e = before:find('<[^>]-$'), before:find('>[^<]-$')
  if s and e then return s > e and index or nil end
  if s then return index end
  return input:find('^[^<]->', index) and index or nil
end)

local equals = token(lexer.OPERATOR, '=') -- * in_tag
-- lex:add_rule('equals', equals)

-- Strings.
local string = #S('\'"') * lexer.last_char_includes('=') *
  token(lexer.STRING, lexer.range("'") + lexer.range('"'))
lex:add_rule('string', string)

-- Numbers.
local number = token(lexer.NUMBER, lexer.dec_num * P('%')^-1)
lex:add_rule('number', #lexer.digit * lexer.last_char_includes('=') * number) -- *in_tag)

-- Entities.
lex:add_rule('entity', token('entity', '&' * (lexer.any - lexer.space - ';')^1 * ';'))
lex:add_style('entity', lexer.styles.comment)

-- Fold points.
local function disambiguate_lt(text, pos, line, s)
  if line:find('/>', s) then
    return 0
  elseif line:find('^</', s) then
    return -1
  else
    return 1
  end
end
lex:add_fold_point('element', '<', disambiguate_lt)
lex:add_fold_point('unknown_element', '<', disambiguate_lt)
lex:add_fold_point(lexer.COMMENT, '<!--', '-->')

-- Tags that start embedded languages.
-- Export these patterns for proxy lexers (e.g. ASP) that need them.
lex.embed_start_tag = element * (ws * attribute * ws^-1 * equals * ws^-1 * string)^0 * ws^-1 *
  tag_close
lex.embed_end_tag = element * tag_close

-- Embedded CSS (<style type="text/css"> ... </style>).
local css = lexer.load('css')
local style_element = word_match('style', true)
local css_start_rule = #('<' * style_element * ('>' + P(function(input, index)
  if input:find('^%s+type%s*=%s*(["\'])text/css%1', index) then return index end
end))) * lex.embed_start_tag
local css_end_rule = #('</' * style_element * ws^-1 * '>') * lex.embed_end_tag
lex:embed(css, css_start_rule, css_end_rule)

-- Embedded JavaScript (<script type="text/javascript"> ... </script>).
local js = lexer.load('javascript')
local script_element = word_match('script', true)
local js_start_rule = #('<' * script_element * ('>' + P(function(input, index)
  if input:find('^%s+type%s*=%s*(["\'])text/javascript%1', index) then return index end
end))) * lex.embed_start_tag
local js_end_rule = #('</' * script_element * ws^-1 * '>') * lex.embed_end_tag
local js_line_comment = '//' * (lexer.nonnewline - js_end_rule)^0
local js_block_comment = '/*' * (lexer.any - '*/' - js_end_rule)^0 * P('*/')^-1
js:modify_rule('comment', token(lexer.COMMENT, js_line_comment + js_block_comment))
lex:embed(js, js_start_rule, js_end_rule)

-- Embedded CoffeeScript (<script type="text/coffeescript"> ... </script>).
local cs = lexer.load('coffeescript')
script_element = word_match('script', true)
local cs_start_rule = #('<' * script_element * P(function(input, index)
  if input:find('^[^>]+type%s*=%s*(["\'])text/coffeescript%1', index) then return index end
end)) * lex.embed_start_tag
local cs_end_rule = #('</' * script_element * ws^-1 * '>') * lex.embed_end_tag
lex:embed(cs, cs_start_rule, cs_end_rule)

return lex
