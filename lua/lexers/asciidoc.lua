-- Copyright 2025 Matěj Cepl. See LICENSE.
-- Copyright 2026 Jamie Drinkell. See LICENSE.
-- Asciidoc LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

-- This lexer does not accomodate Asciidoctor's optional Markdown compatibility.
-- Not all implementations of Asciidoc support it, e.g. asciidoctor-go and asciidoc-hs.
local lex = lexer.new(...)

-- Admonitions.
lex:add_rule('admonition', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD) * #S(':')))
lex:add_rule('block_admonition',
	lex:tag(lexer.KEYWORD, P('[') * lex:word_match(lexer.KEYWORD) * P(']')))

-- Block elements.
local function h(n)
	return lex:tag(string.format('%s.h%s', lexer.HEADING, n),
		lexer.to_eol(lexer.starts_line(string.rep('=', n) * S(' ') * lexer.alnum)))
end
lex:add_rule('header', h(6) + h(5) + h(4) + h(3) + h(2) + h(1))
lex:add_rule('block_title',
	lex:tag(lexer.HEADING, lexer.to_eol(lexer.starts_line('.') * lexer.alnum)))

lex:add_rule('hr',
	lex:tag('hr', lpeg.Cmt(lexer.starts_line(lpeg.C(S("'")), true), function(input, index, c)
		local line = input:match('[^\r\n]*', index):gsub('[ \t]', '')
		if line:find('[^' .. c .. ']') or #line < 2 then return nil end
		return (select(2, input:find('\r?\n', index)) or #input) + 1 -- include \n for eolfilled styles
	end)))

lex:add_rule('list', lex:tag(lexer.LIST, lexer.starts_line(S('*.')^1, true) * S(' \t')))

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

local code = lpeg.Cmt(lpeg.C(P('`')^1), function(input, index, bt)
	-- `foo`, ``foo``, ``foo`bar``, `foo``bar` are all allowed.
	local _, e = input:find('[^`]' .. bt .. '%f[^`]', index)
	return (e or #input) + 1
end)
lex:add_rule('code', lex:tag(lexer.CODE, code))

lex:add_rule('strong', lex:tag(lexer.BOLD, lexer.range('*', true)))
lex:add_rule('em', lex:tag(lexer.ITALIC, lexer.range('_', true)))

-- Handles flanking delimiters as described in
-- https://github.github.com/gfm/#emphasis-and-strong-emphasis in the cases where simple
-- delimited ranges are not sufficient.
local punct_space = lexer.punct + lexer.space
local hspace = lexer.space - '\n'
local blank_line = '\n' * hspace^0 * ('\n' + P(-1))
local function flanked_range(s, not_inword)
	local fl_char = lexer.any - s - lexer.space
	local left_fl = B(punct_space - s) * s * #fl_char + s * #(fl_char - lexer.punct)
	local right_fl = B(lexer.punct) * s * #(punct_space - s) + B(fl_char) * s
	return left_fl * (lexer.any - blank_line - (not_inword and s * #punct_space or s))^0 * right_fl
end

local attribute = flanked_range(':')
lex:add_rule('attribute', lex:tag(lexer.ATTRIBUTE, attribute))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.starts_line(lexer.to_eol('//'))))

-- Word lists.
lex:set_word_list(lexer.KEYWORD, 'NOTE IMPORTANT WARNING TIP CAUTION TESTME')

lexer.property['scintillua.comment'] = '//'

return lex
