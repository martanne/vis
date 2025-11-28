-- Copyright 2006-2025 Mitchell. See LICENSE.
-- WSF LPeg lexer (based on XML).
-- Contributed by Jeff Stone.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.range('<!--', '-->')))

-- Elements.
local identifier = (lexer.alpha + S('_-')) * (lexer.alnum + S('_-'))^0
local tag = lex:tag(lexer.TAG, '<' * P('/')^-1 * identifier)
lex:add_rule('tag', tag)

-- Closing tags.
local tag_close = lex:tag(lexer.TAG, P('/')^-1 * '>')
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
local ws = lex:get_rule('whitespace')
local attribute_eq = lex:tag(lexer.ATTRIBUTE, identifier) * ws^-1 * equals
lex:add_rule('attribute', attribute_eq)

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"', false, false)
local string = lex:tag(lexer.STRING, lexer.after_set('=', sq_str + dq_str))
lex:add_rule('string', string)

-- Numbers.
local number = lex:tag(lexer.NUMBER, lexer.dec_num * P('%')^-1)
lex:add_rule('number', lexer.after_set('=', number)) -- * in_tag)

-- Entities.
local predefined = lex:tag(lexer.CONSTANT_BUILTIN .. '.entity',
	'&' * lexer.word_match('lt gt amp apos quot') * ';')
local general = lex:tag(lexer.CONSTANT .. '.entity', '&' * identifier * ';')
lex:add_rule('entity', predefined + general)

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '<!--', '-->')
local function disambiguate_lt(text, pos, line, s) return not line:find('^</', s) and 1 or -1 end
lex:add_fold_point(lexer.TAG, '<', disambiguate_lt)
lex:add_fold_point(lexer.TAG, '/>', -1)

-- Finally, add JavaScript and VBScript as embedded languages

-- Tags that start embedded languages.
local embed_start_tag = tag * (ws * attribute_eq * ws^-1 * string)^0 * ws^-1 * tag_close
local embed_end_tag = tag * tag_close

-- Embedded JavaScript.
local js = lexer.load('javascript')
local js_start_rule = #(P('<script') * (P(function(input, index)
	if input:find('^%s+language%s*=%s*(["\'])[jJ][ava]*[sS]cript%1', index) then return true end
end) + '>')) * embed_start_tag -- <script language="javascript">
local js_end_rule = #P('</script>') * embed_end_tag -- </script>
lex:embed(js, js_start_rule, js_end_rule)

-- Embedded VBScript.
local vbs = lexer.load('vb', 'vbscript')
local vbs_start_rule = #(P('<script') * (P(function(input, index)
	if input:find('^%s+language%s*=%s*(["\'])[vV][bB][sS]cript%1', index) then return true end
end) + '>')) * embed_start_tag -- <script language="vbscript">
local vbs_end_rule = #P('</script>') * embed_end_tag -- </script>
lex:embed(vbs, vbs_start_rule, vbs_end_rule)

lexer.property['scintillua.comment'] = '<!--|-->'
lexer.property['scintillua.angle.braces'] = '1'

return lex
