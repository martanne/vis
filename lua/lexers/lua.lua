-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Lua LPeg lexer.
-- Original written by Peter Odding, 2007/04/04.

local lexer = lexer
local B, P, S = lpeg.B, lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
local non_field = -B('.') + B('_G.') + B('..')
local builtin_func = lex:word_match(lexer.FUNCTION_BUILTIN)
local lib_func = lex:word_match(lexer.FUNCTION_BUILTIN .. '.library')
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = B(':') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function',
	method + ((non_field * lex:tag(lexer.FUNCTION_BUILTIN, builtin_func + lib_func)) + func) *
		#(lexer.space^0 * S('({\'"')))

-- Constants.
local builtin_const = lex:word_match(lexer.CONSTANT_BUILTIN)
local lib_const = lex:word_match(lexer.CONSTANT_BUILTIN .. '.library')
lex:add_rule('constant', non_field * lex:tag(lexer.CONSTANT_BUILTIN, builtin_const + lib_const))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local longstring = lpeg.Cmt('[' * lpeg.C(P('=')^0) * '[', function(input, index, eq)
	local _, e = input:find(']' .. eq .. ']', index, true)
	return (e or #input) + 1
end)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str) +
	lex:tag(lexer.STRING .. '.longstring', longstring))

-- Comments.
local line_comment = lexer.to_eol('--')
local block_comment = '--' * longstring
lex:add_rule('comment', lex:tag(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
local lua_integer = P('-')^-1 * (lexer.hex_num + lexer.dec_num)
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.float + lua_integer))

-- Labels.
lex:add_rule('label', lex:tag(lexer.LABEL, '::' * lexer.word * '::'))

-- Attributes.
lex:add_rule('attribute', lex:tag(lexer.ATTRIBUTE, '<' * lexer.space^0 *
	lexer.word_match('const close') * lexer.space^0 * '>'))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '..' + S('+-*/%^#=<>&|~;:,.{}[]()')))

-- Fold points.
local function fold_longcomment(text, pos, line, s, symbol)
	if symbol == '[' then
		if line:find('^%[=*%[', s) then return 1 end
	elseif symbol == ']' then
		if line:find('^%]=*%]', s) then return -1 end
	end
	return 0
end
lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
lex:add_fold_point(lexer.KEYWORD, 'do', 'end')
lex:add_fold_point(lexer.KEYWORD, 'function', 'end')
lex:add_fold_point(lexer.KEYWORD, 'repeat', 'until')
lex:add_fold_point(lexer.COMMENT, '[', fold_longcomment)
lex:add_fold_point(lexer.COMMENT, ']', fold_longcomment)
lex:add_fold_point(lexer.FUNCTION .. '.longstring', '[', ']')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', 'function', 'if', 'in', 'local',
	'or', 'nil', 'not', 'repeat', 'return', 'then', 'true', 'until', 'while', --
	'goto' -- 5.2
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'assert', 'collectgarbage', 'dofile', 'error', 'getmetatable', 'ipairs', 'load', 'loadfile',
	'next', 'pairs', 'pcall', 'print', 'rawequal', 'rawget', 'rawset', 'require', 'select',
	'setmetatable', 'tonumber', 'tostring', 'type', 'xpcall', --
	'rawlen', -- 5.2
	'warn' -- 5.4
})

lex:set_word_list(lexer.FUNCTION_BUILTIN .. '.library', {
	'coroutine.create', 'coroutine.resume', 'coroutine.running', 'coroutine.status', 'coroutine.wrap',
	'coroutine.yield', --
	'coroutine.isyieldable', -- 5.3
	'coroutine.close', -- 5.4
	'package.loadlib', --
	'package.searchpath', -- 5.2
	'utf8.char', 'utf8.codepoint', 'utf8.codes', 'utf8.len', 'utf8.offset', -- 5.3
	'string.byte', 'string.char', 'string.dump', 'string.find', 'string.format', 'string.gmatch',
	'string.gsub', 'string.len', 'string.lower', 'string.match', 'string.rep', 'string.reverse',
	'string.sub', 'string.upper', --
	'string.pack', 'string.packsize', 'string.unpack', -- 5.3
	'table.concat', 'table.insert', 'table.remove', 'table.sort', --
	'table.pack', 'table.unpack', -- 5.2
	'table.move', -- 5.3
	'math.abs', 'math.acos', 'math.asin', 'math.atan', 'math.ceil', 'math.cos', 'math.deg',
	'math.exp', 'math.floor', 'math.fmod', 'math.log', 'math.max', 'math.min', 'math.modf',
	'math.rad', 'math.random', 'math.randomseed', 'math.sin', 'math.sqrt', 'math.tan', --
	'math.tointeger', 'math.type', 'math.ult', -- 5.3
	'io.close', 'io.flush', 'io.input', 'io.lines', 'io.open', 'io.output', 'io.popen', 'io.read',
	'io.tmpfile', 'io.type', 'io.write', --
	'os.clock', 'os.date', 'os.difftime', 'os.execute', 'os.exit', 'os.getenv', 'os.remove',
	'os.rename', 'os.setlocale', 'os.time', 'os.tmpname', --
	'debug', 'debug.debug', 'debug.gethook', 'debug.getinfo', 'debug.getlocal', 'debug.getmetatable',
	'debug.getregistry', 'debug.getupvalue', 'debug.sethook', 'debug.setlocal', 'debug.setmetatable',
	'debug.setupvalue', 'debug.traceback', --
	'debug.getuservalue', 'debug.setuservalue', 'debug.upvalueid', 'debug.upvaluejoin' -- 5.2
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'_G', '_VERSION', --
	'_ENV' -- 5.2
})

lex:set_word_list(lexer.CONSTANT_BUILTIN .. '.library', {
	'coroutine', --
	'package', 'package.cpath', 'package.loaded', 'package.path', 'package.preload', --
	'package.config', 'package.searchers', -- 5.2
	'utf8', 'utf8.charpattern', -- 5.3
	'string', --
	'table', --
	'math', 'math.huge', 'math.pi', --
	'math.maxinteger', 'math.mininteger', -- 5.3
	'io', 'io.stderr', 'io.stdin', 'io.stdout', --
	'os'
})

lexer.property['scintillua.comment'] = '--'

return lex
