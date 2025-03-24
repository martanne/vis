-- Copyright 2006-2025 Mitchell. See LICENSE.
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

local code_line = lexer.starts_line((B('    ') + B('\t')) * lpeg.P(function(input, index)
	-- Backtrack to the start of the current paragraph, which is either after a blank line,
	-- at the start of a higher level of indentation, or at the start of the buffer.
	local line, blank_line = lexer.line_from_position(index), false
	while line > 0 do
		local s, e = lexer.line_start[line], lexer.line_end[line]
		blank_line = s == e or lexer.text_range(s, e - s + 1):find('^%s+$')
		if blank_line then break end
		local indent_amount = lexer.indent_amount[line]
		line = line - 1
		if line > 0 and lexer.indent_amount[line] > indent_amount then break end
	end

	-- If the start of the paragraph does not being with a '    ' or '\t', then this line
	-- is a continuation of the current paragraph, not a code block.
	local text = lexer.text_range(lexer.line_start[line + 1], 4)
	if not text:find('^\t') and text ~= '    ' then return false end

	-- If the current paragraph is a code block, then so is this line.
	if line <= 1 then return true end

	-- Backtrack to see if this line is in a list item. If so, it is not a code block.
	while line > 1 do
		line = line - 1
		local s, e = lexer.line_start[line], lexer.line_end[line]
		local blank = s == e or lexer.text_range(s, e - s + 1):find('^%s+$')
		if not blank and lexer.indent_amount[line] == 0 then break end
	end
	text = lexer.text_range(lexer.line_start[line], 8) -- note: only 2 is needed for unordered lists
	if text:find('^[*+-][ \t]') then return false end
	if text:find('^%d+%.[ \t]') then return false end

	return true -- if all else fails, it is probably a code block
end) * lexer.to_eol(), true)

local code_block = lexer.range(lexer.starts_line('```', true),
	'\n' * hspace^0 * '```' * hspace^0 * ('\n' + P(-1))) +
	lexer.range(lexer.starts_line('~~~', true), '\n' * hspace^0 * '~~~' * hspace^0 * ('\n' + P(-1)))

local code_inline = lpeg.Cmt(lpeg.C(P('`')^1), function(input, index, bt)
	-- `foo`, ``foo``, ``foo`bar``, `foo``bar` are all allowed.
	local _, e = input:find('[^`]' .. bt .. '%f[^`]', index)
	return (e or #input) + 1
end)

lex:add_rule('block_code', lex:tag(lexer.CODE, code_line + code_block + code_inline))

lex:add_rule('blockquote', lex:tag(lexer.STRING, lexer.starts_line('>', true)))

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

local FOLD_HEADER, FOLD_BASE = lexer.FOLD_HEADER, lexer.FOLD_BASE
-- Fold '#' headers.
function lex:fold(text, start_line, start_level)
	local levels = {}
	local line_num = start_line
	if start_level > FOLD_HEADER then start_level = start_level - FOLD_HEADER end
	for line in (text .. '\n'):gmatch('(.-)\r?\n') do
		local header = line:match('^%s*(#*)')
		-- If the previous line was a header, this line's level has been pre-defined.
		-- Otherwise, use the previous line's level, or if starting to fold, use the start level.
		local level = levels[line_num] or levels[line_num - 1] or start_level
		if level > FOLD_HEADER then level = level - FOLD_HEADER end
		-- If this line is a header, set its level to be one less than the header level
		-- (so it can be a fold point) and mark it as a fold point.
		if #header > 0 then
			level = FOLD_BASE + #header - 1 + FOLD_HEADER
			levels[line_num + 1] = FOLD_BASE + #header
		end
		levels[line_num] = level
		line_num = line_num + 1
	end
	return levels
end

return lex
