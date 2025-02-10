-- Copyright 2006-2025 Mitchell. See LICENSE.
-- YAML LPeg lexer.
-- It does not keep track of indentation perfectly.

local lexer = lexer
local word_match = lexer.word_match
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(..., {fold_by_indentation = true})

-- Distinguish between horizontal and vertical space so indenting tabs can be marked as errors.
local tab_indent = lex:tag(lexer.ERROR .. '.indent', lexer.starts_line('\t', true))
lex:modify_rule('whitespace', tab_indent + lex:tag(lexer.WHITESPACE, S(' \r\n')^1 + P('\t')^1))

-- Document boundaries.
lex:add_rule('doc_bounds', lex:tag(lexer.OPERATOR, lexer.starts_line(P('---') + '...')))

-- Keys.
local word = (lexer.alnum + '-')^1
lex:add_rule('key', -P('- ') * lex:tag(lexer.STRING, word * (S(' \t_')^1 * word^-1)^0) *
	#P(':' * lexer.space))

-- Collections.
lex:add_rule('collection', lex:tag(lexer.OPERATOR,
	lexer.after_set('?-:\n', S('?-') * #P(' '), ' \t') + ':' * #P(lexer.space) + S('[]{}') + ',' *
		#P(' ')))

-- Alias indicators.
local anchor = lex:tag(lexer.OPERATOR, '&') * lex:tag(lexer.LABEL, word)
local alias = lex:tag(lexer.OPERATOR, '*') * lex:tag(lexer.LABEL, word)
lex:add_rule('alias', anchor + alias)

-- Tags.
local explicit_tag = '!!' * word_match{
	'map', 'omap', 'pairs', 'set', 'seq', -- collection
	'binary', 'bool', 'float', 'int', 'merge', 'null', 'str', 'timestamp', 'value', 'yaml' -- scalar
}
local verbatim_tag = '!' * lexer.range('<', '>', true)
local short_tag = '!' * word * ('!' * (1 - lexer.space)^1)^-1
lex:add_rule('tag', lex:tag(lexer.TYPE, explicit_tag + verbatim_tag + short_tag))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Reserved.
lex:add_rule('reserved',
	B(S(':,') * ' ') * lex:tag(lexer.ERROR, S('@`') + lexer.starts_line(S('@`'))))

-- Constants.
local scalar_end = #(S(' \t')^0 * lexer.newline + S(',]}') + -1)
lex:add_rule('constant',
	lex:tag(lexer.CONSTANT_BUILTIN, word_match('null true false', true)) * scalar_end)

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str) * (scalar_end + #P(':' * lexer.space)))

-- Timestamps.
local year = lexer.digit * lexer.digit * lexer.digit * lexer.digit
local month = lexer.digit * lexer.digit^-1
local day = lexer.digit * lexer.digit^-1
local date = year * '-' * month * '-' * day
local hours = lexer.digit * lexer.digit^-1
local minutes = lexer.digit * lexer.digit
local seconds = lexer.digit * lexer.digit
local fraction = '.' * lexer.digit^0
local time = hours * ':' * minutes * ':' * seconds * fraction^-1
local zone = 'Z' + S(' \t')^-1 * S('-+') * hours * (':' * minutes)^-1
lex:add_rule('timestamp', lex:tag(lexer.NUMBER .. '.timestamp',
	date * (S('tT \t') * time * zone^-1)^-1) * scalar_end)

-- Numbers.
local special_num = S('+-')^-1 * '.' * word_match('inf nan', true)
local number = lexer.number + special_num
lex:add_rule('number', (B(lexer.alnum) * lex:tag(lexer.DEFAULT, number) +
	lex:tag(lexer.NUMBER, number)) * scalar_end)

-- Scalars.
local block_indicator = S('|>') * (S('-+') * lexer.digit^-1 + lexer.digit * S('-+')^-1)^-1
local block = lpeg.Cmt(lpeg.C(block_indicator * lexer.newline), function(input, index, indicator)
	local indent = lexer.indent_amount[lexer.line_from_position(index - #indicator)]
	for s, i, j in input:gmatch('()\n()[ \t]*()[^ \t\r\n]', index) do -- ignore blank lines
		if s >= index then -- compatibility for Lua < 5.4, which doesn't have init for string.gmatch()
			if j - i <= indent then return s end
		end
	end
	return #input + 1
end)
local seq = B('- ') * lexer.nonnewline^1
local csv = B(', ') * (lexer.nonnewline - S(',]}'))^1
local stop_chars, LF = {[string.byte('{')] = true, [string.byte('\n')] = true}, string.byte('\n')
local map = B(': ') * lexer.nonnewline * P(function(input, index)
	local pos = index
	while pos > 1 and not stop_chars[input:byte(pos)] do pos = pos - 1 end
	local s = input:find(input:byte(pos) ~= LF and '[\n,}]' or '\n', index)
	return s or #input + 1
end)
lex:add_rule('scalar', lex:tag(lexer.DEFAULT, block + seq + csv + map))

-- Directives
lex:add_rule('directive', lex:tag(lexer.PREPROCESSOR, lexer.starts_line(lexer.to_eol('%'))))

lexer.property['scintillua.comment'] = '#'

return lex
