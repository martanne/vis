-- Copyright 2006-2025 Mitchell. See LICENSE.
-- JavaScript LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Functions.
local builtin_func = -B('.') *
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
local string = lex:tag(lexer.STRING, sq_str + dq_str + bq_str)
local regex_str = lexer.after_set('+-*%^!=&|?:;,([{<>', lexer.range('/', true) * S('igm')^0)
local regex = lex:tag(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%^!=&|?:;,.()[]{}<>')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'abstract', 'async', 'await', 'boolean', 'break', 'byte', 'case', 'catch', 'char', 'class',
	'const', 'continue', 'debugger', 'default', 'delete', 'do', 'double', 'else', 'enum', 'export',
	'extends', 'false', 'final', 'finally', 'float', 'for', 'function', 'get', 'goto', 'if',
	'implements', 'import', 'in', 'instanceof', 'int', 'interface', 'let', 'long', 'native', 'new',
	'null', 'of', 'package', 'private', 'protected', 'public', 'return', 'set', 'short', 'static',
	'super', 'switch', 'synchronized', 'this', 'throw', 'throws', 'transient', 'true', 'try',
	'typeof', 'var', 'void', 'volatile', 'while', 'with', 'yield'
})

lex:set_word_list(lexer.TYPE, {
	-- Fundamental objects.
	'Object', 'Function', 'Boolean', 'Symbol',
	-- Error Objects.
	'Error', 'AggregateError', 'EvalError', 'InternalError', 'RangeError', 'ReferenceError',
	'SyntaxError', 'TypeError', 'URIError',
	-- Numbers and dates.
	'Number', 'BigInt', 'Math', 'Date',
	-- Text Processing.
	'String', 'RegExp',
	-- Indexed collections.
	'Array', 'Int8Array', 'Uint8Array', 'Uint8ClampedArray', 'Int16Array', 'Uint16Array',
	'Int32Array', 'Uint32Array', 'Float32Array', 'Float64Array', 'BigInt64Array', 'BigUint64Array',
	-- Keyed collections.
	'Map', 'Set', 'WeakMap', 'WeakSet',
	-- Structured data.
	'ArrayBuffer', 'SharedArrayBuffer', 'Atomics', 'DataView', 'JSON',
	-- Control abstraction objects.
	'GeneratorFunction', 'AsyncGeneratorFunction', 'Generator', 'AsyncGenerator', 'AsyncFunction',
	'Promise',
	-- Reflection.
	'Reflect', 'Proxy',
	-- Other.
	'Intl', 'WebAssembly'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'eval', 'isFinite', 'isNaN', 'parseFloat', 'parseInt', 'decodeURI', 'decodeURIComponent',
	'encodeURI', 'encodeURIComponent'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, 'Infinity NaN undefined globalThis arguments')

lexer.property['scintillua.comment'] = '//'

return lex
