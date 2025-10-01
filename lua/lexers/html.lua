-- Copyright 2006-2025 Mitchell. See LICENSE.
-- HTML LPeg lexer.

local lexer = lexer
local word_match = lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {no_user_word_lists = true})

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.range('<!--', '-->')))

-- Doctype.
lex:add_rule('doctype',
	lex:tag(lexer.TAG .. '.doctype', lexer.range('<!' * word_match('doctype', true), '>')))

-- Tags.
local paired_tag = lex:tag(lexer.TAG, lex:word_match(lexer.TAG, true))
local single_tag = lex:tag(lexer.TAG .. '.single', lex:word_match(lexer.TAG .. '.single', true))
local known_tag = paired_tag + single_tag
local unknown_tag = lex:tag(lexer.TAG .. '.unknown', (lexer.alnum + '-')^1)
local tag = lex:tag(lexer.TAG .. '.chars', '<' * P('/')^-1) * (known_tag + unknown_tag) * -P(':')
lex:add_rule('tag', tag)

-- Closing tags.
local tag_close = lex:tag(lexer.TAG .. '.chars', P('/')^-1 * '>')
lex:add_rule('tag_close', tag_close)

-- Equals.
-- TODO: performance is terrible on large files.
local in_tag = P(function(input, index)
	local before = input:sub(1, index - 1)
	local s, e = before:find('<[^>]-$'), before:find('>[^<]-$')
	if s and e then return s > e end
	if s then return true end
	return input:find('^[^<]->', index) ~= nil
end)

local equals = lex:tag(lexer.OPERATOR, '=') -- * in_tag
-- lex:add_rule('equals', equals)

-- Attributes.
local known_attribute = lex:tag(lexer.ATTRIBUTE, lex:word_match(lexer.ATTRIBUTE, true) +
	((P('data-') + 'aria-') * (lexer.alnum + '-')^1))
local unknown_attribute = lex:tag(lexer.ATTRIBUTE .. '.unknown', (lexer.alnum + '-')^1)
local ws = lex:get_rule('whitespace')
local attribute_eq = (known_attribute + unknown_attribute) * ws^-1 * equals
lex:add_rule('attribute', attribute_eq)

-- Strings.
local string = lex:tag(lexer.STRING, lexer.after_set('=', lexer.range("'") + lexer.range('"')))
lex:add_rule('string', string)

-- Numbers.
local number = lex:tag(lexer.NUMBER, lexer.dec_num * P('%')^-1)
lex:add_rule('number', lexer.after_set('=', number)) -- *in_tag)

-- Entities.
lex:add_rule('entity', lex:tag(lexer.CONSTANT_BUILTIN .. '.entity',
	'&' * (lexer.any - lexer.space - ';')^1 * ';'))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '<!--', '-->')
local function disambiguate_lt(text, pos, line, s)
	if line:find('/>', s) then
		return 0
	elseif line:find('^</', s) then
		return -1
	else
		return 1
	end
end
lex:add_fold_point(lexer.TAG .. '.chars', '<', disambiguate_lt)

-- Tags that start embedded languages.
-- Export these patterns for proxy lexers (e.g. ASP) that need them.
lex.embed_start_tag = tag * (ws * attribute_eq * ws^-1 * string)^0 * ws^-1 * tag_close
lex.embed_end_tag = tag * tag_close

-- Embedded CSS (<style type="text/css"> ... </style>).
local css = lexer.load('css')
local style_tag = word_match('style', true)
local css_start_rule = #('<' * style_tag * ('>' + P(function(input, index)
	if input:find('^%s+type%s*=%s*(["\'])text/css%1', index) then return true end
end))) * lex.embed_start_tag
local css_end_rule = #('</' * style_tag * '>') * lex.embed_end_tag
lex:embed(css, css_start_rule, css_end_rule)
-- Embedded CSS in style="" attribute.
local style = lexer.load('css', 'css.style')
css_start_rule = #(P('style') * lexer.space^0 * '=') * attribute_eq * ws^-1 *
	lex:tag(lexer.STRING, '"')
css_end_rule = lex:tag(lexer.STRING, '"')
lex:embed(style, css_start_rule, css_end_rule) -- only double-quotes for now

-- Embedded JavaScript (<script type="text/javascript"> ... </script>).
local js = lexer.load('javascript')
local script_tag = word_match('script', true)
local js_start_rule = #('<' * script_tag * ('>' + P(function(input, index)
	if input:find('^%s+type%s*=%s*(["\'])text/javascript%1', index) then return true end
end))) * lex.embed_start_tag
local js_end_rule = #('</' * script_tag * '>') * lex.embed_end_tag
lex:embed(js, js_start_rule, js_end_rule)

-- Embedded CoffeeScript (<script type="text/coffeescript"> ... </script>).
local cs = lexer.load('coffeescript')
script_tag = word_match('script', true)
local cs_start_rule = #('<' * script_tag * P(function(input, index)
	if input:find('^[^>]+type%s*=%s*(["\'])text/coffeescript%1', index) then return true end
end)) * lex.embed_start_tag
local cs_end_rule = #('</' * script_tag * '>') * lex.embed_end_tag
lex:embed(cs, cs_start_rule, cs_end_rule)

-- Word lists.
lex:set_word_list(lexer.TAG .. '.single', {
	'area', 'base', 'br', 'col', 'command', 'embed', 'hr', 'img', 'input', 'keygen', 'link', 'meta',
	'param', 'source', 'track', 'wbr'
})

lex:set_word_list(lexer.TAG, {
	'a', 'abbr', 'address', 'article', 'aside', 'audio', 'b', 'bdi', 'bdo', 'blockquote', 'body',
	'button', 'canvas', 'caption', 'cite', 'code', 'colgroup', 'content', 'data', 'datalist', 'dd',
	'decorator', 'del', 'details', 'dfn', 'div', 'dl', 'dt', 'element', 'em', 'fieldset',
	'figcaption', 'figure', 'footer', 'form', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head', 'header',
	'html', 'i', 'iframe', 'ins', 'kbd', 'label', 'legend', 'li', 'main', 'map', 'mark', 'menu',
	'menuitem', 'meter', 'nav', 'noscript', 'object', 'ol', 'optgroup', 'option', 'output', 'p',
	'pre', 'progress', 'q', 'rp', 'rt', 'ruby', 's', 'samp', 'script', 'section', 'select', 'shadow',
	'small', 'spacer', 'span', 'strong', 'style', 'sub', 'summary', 'sup', 'table', 'tbody', 'td',
	'template', 'textarea', 'tfoot', 'th', 'thead', 'time', 'title', 'tr', 'u', 'ul', 'var', 'video'
})

lex:set_word_list(lexer.ATTRIBUTE, {
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
	'step', 'style', 'summary', 'tabindex', 'target', 'title', 'type', 'usemap', 'value', 'width',
	'wrap'
})

lexer.property['scintillua.comment'] = '<!--|-->'
lexer.property['scintillua.angle.braces'] = '1'
lexer.property['scintillua.word.chars'] =
	'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-'

return lex
