-- Copyright 2006-2025 Mitchell. See LICENSE.
-- AWK LPeg lexer.
-- Modified by Wolfgang Seeberg 2012, 2013.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

local LEFTBRACKET = '['
local RIGHTBRACKET = ']'
local SLASH = '/'
local BACKSLASH = '\\'
local CARET = '^'
local CR = '\r'
local LF = '\n'
local CRLF = CR .. LF
local DQUOTE = '"'
local DELIMITER_MATCHES = {['('] = ')', ['['] = ']'}
local COMPANION = {['('] = '[', ['['] = '('}
local CC = {
	alnum = 1, alpha = 1, blank = 1, cntrl = 1, digit = 1, graph = 1, lower = 1, print = 1, punct = 1,
	space = 1, upper = 1, xdigit = 1
}
local LastRegexEnd = 0
local BackslashAtCommentEnd = 0
local KW_BEFORE_RX = {
	case = 1, ['do'] = 1, ['else'] = 1, exit = 1, print = 1, printf = 1, ['return'] = 1
}

local function findKeyword(input, e)
	local i = e
	while i > 0 and input:find("^[%l]", i) do i = i - 1 end
	local w = input:sub(i + 1, e)
	if i == 0 then
		return KW_BEFORE_RX[w] == 1
	elseif input:find("^[%u%d_]", i) then
		return false
	else
		return KW_BEFORE_RX[w] == 1
	end
end

local function isRegex(input, i)
	while i >= 1 and input:find('^[ \t]', i) do i = i - 1 end
	if i < 1 then return true end
	if input:find("^[-!%%&(*+,:;<=>?[^{|}~\f]", i) or findKeyword(input, i) then
		return true
	elseif input:sub(i, i) == SLASH then
		return i ~= LastRegexEnd -- deals with /xx/ / /yy/.
	elseif input:find('^[]%w)."]', i) then
		return false
	elseif input:sub(i, i) == LF then
		if i == 1 then return true end
		i = i - 1
		if input:sub(i, i) == CR then
			if i == 1 then return true end
			i = i - 1
		end
	elseif input:sub(i, i) == CR then
		if i == 1 then return true end
		i = i - 1
	else
		return false
	end
	if input:sub(i, i) == BACKSLASH and i ~= BackslashAtCommentEnd then
		return isRegex(input, i - 1)
	else
		return true
	end
end

local function eatCharacterClass(input, s, e)
	local i = s
	while i <= e do
		if input:find('^[\r\n]', i) then
			return false
		elseif input:sub(i, i + 1) == ':]' then
			local str = input:sub(s, i - 1)
			return CC[str] == 1 and i + 1
		end
		i = i + 1
	end
	return false
end

local function eatBrackets(input, i, e)
	if input:sub(i, i) == CARET then i = i + 1 end
	if input:sub(i, i) == RIGHTBRACKET then i = i + 1 end
	while i <= e do
		if input:find('^[\r\n]', i) then
			return false
		elseif input:sub(i, i) == RIGHTBRACKET then
			return i
		elseif input:sub(i, i + 1) == '[:' then
			i = eatCharacterClass(input, i + 2, e)
			if not i then return false end
		elseif input:sub(i, i) == BACKSLASH then
			i = i + 1
			if input:sub(i, i + 1) == CRLF then i = i + 1 end
		end
		i = i + 1
	end
	return false
end

local function eatRegex(input, i)
	local e = #input
	while i <= e do
		if input:find('^[\r\n]', i) then
			return false
		elseif input:sub(i, i) == SLASH then
			LastRegexEnd = i
			return i
		elseif input:sub(i, i) == LEFTBRACKET then
			i = eatBrackets(input, i + 1, e)
			if not i then return false end
		elseif input:sub(i, i) == BACKSLASH then
			i = i + 1
			if input:sub(i, i + 1) == CRLF then i = i + 1 end
		end
		i = i + 1
	end
	return false
end

local ScanRegexResult
local function scanGawkRegex(input, index)
	if isRegex(input, index - 2) then
		local i = eatRegex(input, index)
		if not i then
			ScanRegexResult = false
			return false
		end
		local rx = input:sub(index - 1, i)
		for bs in rx:gmatch("[^\\](\\+)[BSsWwy<>`']") do
			-- /\S/ is special, but /\\S/ is not.
			if #bs % 2 == 1 then return i + 1 end
		end
		ScanRegexResult = i + 1
	else
		ScanRegexResult = false
	end
	return false
end
-- Is only called immediately after scanGawkRegex().
local function scanRegex() return ScanRegexResult end

local function scanString(input, index)
	local i = index
	local e = #input
	while i <= e do
		if input:find('^[\r\n]', i) then
			return false
		elseif input:sub(i, i) == DQUOTE then
			return i + 1
		elseif input:sub(i, i) == BACKSLASH then
			i = i + 1
			-- lexer.range() doesn't handle CRLF.
			if input:sub(i, i + 1) == CRLF then i = i + 1 end
		end
		i = i + 1
	end
	return false
end

-- purpose: prevent isRegex() from entering a comment line that ends with a backslash.
local function scanComment(input, index)
	local _, i = input:find('[^\r\n]*', index)
	if input:sub(i, i) == BACKSLASH then BackslashAtCommentEnd = i end
	return i + 1
end

local function scanFieldDelimiters(input, index)
	local i = index
	local e = #input
	local left = input:sub(i - 1, i - 1)
	local count = 1
	local right = DELIMITER_MATCHES[left]
	local left2 = COMPANION[left]
	local count2 = 0
	local right2 = DELIMITER_MATCHES[left2]
	while i <= e do
		if input:find('^[#\r\n]', i) then
			return false
		elseif input:sub(i, i) == right then
			count = count - 1
			if count == 0 then return count2 == 0 and i + 1 end
		elseif input:sub(i, i) == left then
			count = count + 1
		elseif input:sub(i, i) == right2 then
			count2 = count2 - 1
			if count2 < 0 then return false end
		elseif input:sub(i, i) == left2 then
			count2 = count2 + 1
		elseif input:sub(i, i) == DQUOTE then
			i = scanString(input, i + 1)
			if not i then return false end
			i = i - 1
		elseif input:sub(i, i) == SLASH then
			if isRegex(input, i - 1) then
				i = eatRegex(input, i + 1)
				if not i then return false end
			end
		elseif input:sub(i, i) == BACKSLASH then
			if input:sub(i + 1, i + 2) == CRLF then
				i = i + 2
			elseif input:find('^[\r\n]', i + 1) then
				i = i + 1
			end
		end
		i = i + 1
	end
	return false
end

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, '#' * P(scanComment)))

-- Strings.
lex:add_rule('string', lex:tag(lexer.STRING, DQUOTE * P(scanString)))

-- No leading sign because it might be binary.
local float = ((lexer.digit^1 * ('.' * lexer.digit^0)^-1) + ('.' * lexer.digit^1)) *
	(S('eE') * S('+-')^-1 * lexer.digit^1)^-1

-- Fields. E.g. $1, $a, $(x), $a(x), $a[x], $"1", $$a, etc.
lex:add_rule('field', lex:tag(lexer.VARIABLE .. '.field', '$' * S('$+-')^0 *
	(float + lexer.word^0 * '(' * P(scanFieldDelimiters) + lexer.word^1 *
		('[' * P(scanFieldDelimiters))^-1 + '"' * P(scanString) + '/' * P(eatRegex) * '/')))

-- Regular expressions.
-- Slash delimited regular expressions are preceded by most operators or the keywords 'print'
-- and 'case', possibly on a preceding line. They can contain unescaped slashes and brackets
-- in brackets. Some escape sequences like '\S', '\s' have special meanings with Gawk. Tokens
-- that contain them are displayed differently.
lex:add_rule('gawkRegex', lex:tag(lexer.REGEX .. '.gawk', SLASH * P(scanGawkRegex)))
lex:add_rule('regex', lex:tag(lexer.REGEX, SLASH * P(scanRegex)))

-- Operators.
lex:add_rule('gawkOperator', lex:tag(lexer.OPERATOR .. '.gawk', P("|&") + "@" + "**=" + "**"))
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('!%&()*+,-/:;<=>?[\\]^{|}~')))

-- Numbers.
lex:add_rule('gawkNumber', lex:tag(lexer.NUMBER .. '.gawk', lexer.hex_num + lexer.oct_num))
lex:add_rule('number', lex:tag(lexer.NUMBER, float))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

lex:add_rule('builtInVariable',
	lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN)))

lex:add_rule('gawkBuiltInVariable', lex:tag(lexer.VARIABLE_BUILTIN .. '.gawk',
	lex:word_match(lexer.VARIABLE_BUILTIN .. '.gawk')))

-- Functions.
local builtin_func = lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
lex:add_rule('function', (builtin_func + func) * #P('('))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'BEGIN', 'END', 'break', 'continue', 'do', 'else', 'for', 'if', 'in', 'while', --
	'delete', -- array
	'print', 'printf', 'getline', 'close', 'fflush', 'system', -- I/O
	'function', 'return', -- functions
	'next', 'nextfile', 'exit' -- program execution
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'gsub', 'index', 'length', 'match', 'split', 'sprintf', 'sub', 'substr', 'tolower', 'toupper', -- string
	'mktime', 'strftime', 'systime', -- time
	'atan2', 'cos', 'exp', 'int', 'log', 'rand', 'sin', 'sqrt', 'srand' -- arithmetic
})

lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'ARGC', 'ARGV', 'CONVFMT', 'ENVIRON', 'FILENAME', 'FNR', 'FS', 'NF', 'NR', 'OFMT', 'OFS', 'ORS',
	'RLENGTH', 'RS', 'RSTART', 'SUBSEP'
})

lex:set_word_list(lexer.VARIABLE_BUILTIN .. '.gawk', {
	'ARGIND', 'BINMODE', 'ERRNO', 'FIELDWIDTHS', 'FPAT', 'FUNCTAB', 'IGNORECASE', 'LINT', 'PREC',
	'PROCINFO', 'ROUNDMODE', 'RT', 'SYMTAB', 'TEXTDOMAIN'
})

lexer.property['scintillua.comment'] = '#'

return lex
