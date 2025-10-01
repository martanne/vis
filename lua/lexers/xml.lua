-- Copyright 2006-2025 Mitchell. See LICENSE.
-- XML LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Comments and CDATA.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.range('<!--', '-->')))
lex:add_rule('cdata', lex:tag('cdata', lexer.range('<![CDATA[', ']]>')))

-- Doctype.
local ws = lex:get_rule('whitespace')
local identifier = (lexer.alpha + S('_-')) * (lexer.alnum + S('_-'))^0
local doctype = lex:tag(lexer.TAG .. '.doctype', '<!DOCTYPE') * ws *
	lex:tag(lexer.TAG .. '.doctype', identifier) * (ws * identifier)^-1 * (1 - P('>'))^0 *
	lex:tag(lexer.TAG .. '.doctype', '>')
lex:add_rule('doctype', doctype)

-- Processing instructions.
lex:add_rule('proc_insn', lex:tag(lexer.TAG .. '.pi', '<?' * identifier + '?>'))

-- Tags.
local namespace = lex:tag(lexer.OPERATOR, ':') * lex:tag(lexer.LABEL, identifier)
lex:add_rule('element', lex:tag(lexer.TAG, '<' * P('/')^-1 * identifier) * namespace^-1)

-- Closing tags.
lex:add_rule('close_tag', lex:tag(lexer.TAG, P('/')^-1 * '>'))

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
-- lex:add_rule('equal', equals)

-- Attributes.
local attribute_eq = lex:tag(lexer.ATTRIBUTE, identifier) * namespace^-1 * ws^-1 * equals
lex:add_rule('attribute', attribute_eq)

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"', false, false)
lex:add_rule('string', lex:tag(lexer.STRING, lexer.after_set('=', sq_str + dq_str)))

-- Numbers.
local number = lex:tag(lexer.NUMBER, lexer.dec_num * P('%')^-1)
lex:add_rule('number', lexer.after_set('=', number)) -- *in_tag)

-- Entities.
local predefined = lex:tag(lexer.CONSTANT_BUILTIN .. '.entity',
	'&' * lexer.word_match('lt gt amp apos quot') * ';')
local general = lex:tag(lexer.CONSTANT .. '.entity', '&' * identifier * ';')
lex:add_rule('entity', predefined + general)

-- Fold Points.
lex:add_fold_point(lexer.COMMENT, '<!--', '-->')
lex:add_fold_point('cdata', '<![CDATA[', ']]>')
local function disambiguate_lt(text, pos, line, s) return not line:find('^</', s) and 1 or -1 end
lex:add_fold_point(lexer.TAG, '<', disambiguate_lt)
lex:add_fold_point(lexer.TAG, '/>', -1)
lex:add_fold_point(lexer.TAG, '?>', -1)

lexer.property['scintillua.comment'] = '<!--|-->'
lexer.property['scintillua.angle.braces'] = '1'
lexer.property['scintillua.word.chars'] =
	'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-'

return lex
