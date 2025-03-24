-- Copyright 2006-2025 Mitchell. See LICENSE.
-- reStructuredText LPeg lexer.

local lexer = lexer
local starts_line = lexer.starts_line
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

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
lex:add_rule('literal_block', lex:tag(lexer.LABEL .. '.literal', block))

-- Lists.
local option_word = lexer.alnum * (lexer.alnum + '-')^0
local option = S('-/') * option_word * (' ' * option_word)^-1 +
	('--' * option_word * ('=' * option_word)^-1)
local option_list = option * (',' * lexer.space^1 * option)^-1
local bullet_list = S('*+-') -- TODO: '•‣⁃', as lpeg does not support UTF-8
local enum_list = P('(')^-1 * (lexer.digit^1 + S('ivxlcmIVXLCM')^1 + lexer.alnum + '#') * S('.)')
local field_list = ':' * (lexer.any - ':')^1 * P(':')^-1
lex:add_rule('list',
	#(lexer.space^0 * (S('*+-:/') + enum_list)) * starts_line(lex:tag(lexer.LIST, lexer.space^0 *
		(option_list + bullet_list + enum_list + field_list) * lexer.space)))

local any_indent = S(' \t')^0
local word = lexer.alpha * (lexer.alnum + S('-.+'))^0
local prefix = any_indent * '.. '

-- Explicit markup blocks.
local footnote_label = '[' * (lexer.digit^1 + '#' * word^-1 + '*') * ']'
local footnote = lex:tag(lexer.LABEL .. '.footnote', prefix * footnote_label * lexer.space)
local citation_label = '[' * word * ']'
local citation = lex:tag(lexer.LABEL .. '.citation', prefix * citation_label * lexer.space)
local link = lex:tag(lexer.LABEL .. '.link', prefix * '_' *
	(lexer.range('`') + (P('\\') * 1 + lexer.nonnewline - ':')^1) * ':' * lexer.space)
lex:add_rule('markup_block', #prefix * starts_line(footnote + citation + link))

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
lex:add_rule('code_block', #prefix * lex:tag(lexer.LABEL .. '.code', starts_line(code_block)))

-- Directives.
local known_directive = lex:tag(lexer.KEYWORD,
	prefix * lex:word_match(lexer.KEYWORD) * '::' * lexer.space)
local sphinx_directive = lex:tag(lexer.KEYWORD .. '.sphinx', prefix *
	lex:word_match(lexer.KEYWORD .. '.sphinx') * '::' * lexer.space)
local unknown_directive = lex:tag(lexer.KEYWORD .. '.unknown', prefix * word * '::' * lexer.space)
lex:add_rule('directive',
	#prefix * starts_line(known_directive + sphinx_directive + unknown_directive))

-- Substitution definitions.
lex:add_rule('substitution',
	#prefix * lex:tag(lexer.FUNCTION, starts_line(prefix * lexer.range('|') * lexer.space^1 * word *
		'::' * lexer.space)))

-- Comments.
local line_comment = lexer.to_eol(prefix)
local bprefix = any_indent * '..'
local block_comment = bprefix * lexer.newline * indented_block
lex:add_rule('comment', #bprefix * lex:tag(lexer.COMMENT, starts_line(line_comment + block_comment)))

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
lex:add_rule('title', lex:tag(lexer.HEADING, overline + underline))

-- Line block.
lex:add_rule('line_block_char', lex:tag(lexer.OPERATOR, starts_line(any_indent * '|')))

-- Whitespace.
lex:add_rule('whitespace', lex:tag(lexer.WHITESPACE, S(' \t')^1 + lexer.newline^1))

-- Inline markup.
local strong = lex:tag(lexer.BOLD, lexer.range('**'))
local em = lex:tag(lexer.ITALIC, lexer.range('*'))
local inline_literal = lex:tag(lexer.CODE .. '.inline', lexer.range('``'))
local postfix_link = (word + lexer.range('`')) * '_' * P('_')^-1
local prefix_link = '_' * lexer.range('`')
local link_ref = lex:tag(lexer.LINK, postfix_link + prefix_link)
local role = lex:tag(lexer.FUNCTION_BUILTIN, ':' * word * ':' * (word * ':')^-1)
local interpreted = role^-1 * lex:tag(lexer.EMBEDDED, lexer.range('`')) * role^-1
local footnote_ref = lex:tag(lexer.REFERENCE, footnote_label * '_')
local citation_ref = lex:tag(lexer.REFERENCE, citation_label * '_')
local substitution_ref = lex:tag(lexer.FUNCTION, lexer.range('|', true) * ('_' * P('_')^-1)^-1)
local link = lex:tag(lexer.LINK,
	lexer.alpha * (lexer.alnum + S('-.'))^1 * ':' * (lexer.alnum + S('/.+-%@'))^1)
lex:add_rule('inline_markup',
	(strong + em + inline_literal + link_ref + interpreted + footnote_ref + citation_ref +
		substitution_ref + link) * -lexer.alnum)

-- Other.
lex:add_rule('non_space', lex:tag(lexer.DEFAULT, lexer.alnum * (lexer.any - lexer.space)^0))
lex:add_rule('escape', lex:tag(lexer.DEFAULT, '\\' * lexer.any))

-- Section-based folding.
local sphinx_levels = {
	['#'] = 0, ['*'] = 1, ['='] = 2, ['-'] = 3, ['^'] = 4, ['"'] = 5
}

function lex:fold(text, start_line, start_level)
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
		if style_at[pos - 1] == CONSTANT and c:find('^[^%w%s]') then
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

--[[ Embedded languages.
local bash = lexer.load('bash')
local bash_indent_level
local start_rule =
  #(prefix * 'code-block' * '::' * lexer.space^1 * 'bash' * (lexer.newline + -1)) *
  sphinx_directive * lex:tag(lexer.EMBEDDED, P(function(input, index)
    bash_indent_level = #input:match('^([ \t]*)', index)
    return index
  end))]]

-- Word lists
lex:set_word_list(lexer.KEYWORD, {
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
})

lex:set_word_list(lexer.KEYWORD .. '.sphinx', {
	-- The TOC tree.
	'toctree',
	-- Paragraph-level markup.
	'note', 'warning', 'versionadded', 'versionchanged', 'deprecated', 'seealso', 'rubric',
	'centered', 'hlist', 'glossary', 'productionlist',
	-- Showing code examples.
	'highlight', 'literalinclude',
	-- Miscellaneous
	'sectionauthor', 'index', 'only', 'tabularcolumns'
})

-- lexer.property['fold.by.sphinx.convention'] = '0'
lexer.property['scintillua.comment'] = '.. '

return lex
