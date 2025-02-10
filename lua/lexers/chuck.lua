-- Copyright 2010-2025 Martin Morawetz. See LICENSE.
-- ChucK LPeg lexer.

local lexer = lexer
local word_match = lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Classes.
lex:add_rule('class', lex:tag(lexer.CLASS, lex:word_match(lexer.CLASS)))

-- Functions.
local std = 'Std.' * lex:word_match(lexer.FUNCTION_BUILTIN)
local machine = 'Machine.' * lex:word_match(lexer.FUNCTION_BUILTIN .. '.machine')
local math = 'Math.' * lex:word_match(lexer.FUNCTION_BUILTIN .. '.math')
local func = lex:tag(lexer.FUNCTION, lexer.word) * #P('(')
lex:add_rule('function', lex:tag(lexer.FUNCTION_BUILTIN, std + machine + math) + func)

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN,
	'Math.' * lex:word_match(lexer.CONSTANT_BUILTIN .. '.math')))

-- Global ugens.
lex:add_rule('ugen', lex:tag(lexer.CONSTANT_BUILTIN .. '.ugen', word_match('dac adc blackhole')))

-- Times.
lex:add_rule('time', lex:tag(lexer.NUMBER, word_match('samp ms second minute hour day week')))

-- Special special value.
lex:add_rule('now', lex:tag(lexer.CONSTANT_BUILTIN .. '.now', word_match('now')))

-- Strings.
local sq_str = P('L')^-1 * lexer.range("'", true)
local dq_str = P('L')^-1 * lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}@')))

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	-- Control structures.
	'break', 'continue', 'else', 'for', 'if', 'repeat', 'return', 'switch', 'until', 'while',
	-- Other chuck keywords.
	'function', 'fun', 'spork', 'const', 'new'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'false', 'maybe', 'me', 'null', 'NULL', 'pi', 'true' -- special values
})

lex:set_word_list(lexer.TYPE, 'float int time dur void same')

-- Class keywords.
lex:set_word_list(lexer.CLASS, {
	'class', 'extends', 'implements', 'interface', 'private', 'protected', 'public', 'pure', 'static',
	'super', 'this'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'abs', 'fabs', 'sgn', 'system', 'atoi', 'atof', 'getenv', 'setenv', 'mtof', 'ftom', 'powtodb',
	'rmstodb', 'dbtopow', 'dbtorms'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN .. '.machine', {
	'add', 'spork', 'remove', 'replace', 'status', 'crash'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN .. '.math', {
	'sin', 'cos', 'tan', 'asin', 'acos', 'atan', 'atan2', 'sinh', 'cosh', 'tanh', 'hypot', 'pow',
	'sqrt', 'exp', 'log', 'log2', 'log10', 'random', 'random2', 'randomf', 'random2f', 'srandom',
	'floor', 'ceil', 'round', 'trunc', 'fmod', 'remainder', 'min', 'max', 'nextpow2', 'isinf', 'isnan'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN .. '.math', {
	'PI', 'TWO_PI', 'e', 'E', 'i', 'I', 'j', 'J', 'RANDOM_MAX'
})

lexer.property['scintillua.comment'] = '//'

return lex
