-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Python LPeg lexer.

local lexer = lexer
local token, word_match = lexer.token, lexer.word_match
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(..., {fold_by_indentation = true})

-- Classes.
lex:add_rule('classdef', lex:tag(lexer.KEYWORD, 'class') * lex:get_rule('whitespace') *
	lex:tag(lexer.CLASS, lexer.word))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)) +
	lex:tag(lexer.KEYWORD .. '.soft', lex:word_match(lexer.KEYWORD .. '.soft')))

-- Functions.
local builtin_func = -B('.') *
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local special_func = lex:tag(lexer.FUNCTION_BUILTIN .. '.special',
	lex:word_match(lexer.FUNCTION_BUILTIN .. '.special'))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + special_func + method + func) * #(lexer.space^0 * '('))

-- Constants.
local builtin_const = lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN))
local attr = lex:tag(lexer.ATTRIBUTE, B('.') * lex:word_match(lexer.ATTRIBUTE) + '__name__')
lex:add_rule('constant', builtin_const + attr)

-- Strings.
-- Note: https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local tq_str = lexer.range("'''") + lexer.range('"""')
lex:add_rule('string', lex:tag(lexer.STRING, (S('rRbBfFtT') * S('rRbBfFtT') + S('rRuUbBfFtT'))^-1 *
	(tq_str + sq_str + dq_str)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#', true)))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_') * S('jJ')^-1))

-- Decorators.
lex:add_rule('decorator', lex:tag(lexer.ANNOTATION, '@' * lexer.word))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('!@%^&*()[]{}-=+/|:;.,<>~')))

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'and', 'as', 'assert', 'async', 'await', 'break', 'class', 'continue', 'def', 'del', 'elif',
	'else', 'except', 'False', 'finally', 'for', 'from', 'global', 'if', 'import', 'in', 'is',
	'lambda', 'None', 'nonlocal', 'not', 'or', 'pass', 'raise', 'return', 'True', 'try', 'while',
	'with', 'yield'
})

lex:set_word_list(lexer.KEYWORD .. '.soft', '_ case match')

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'abs', 'aiter', 'all', 'any', 'anext', 'ascii', 'bin', 'bool', 'breakpoint', 'bytearray', 'bytes',
	'callable', 'chr', 'classmethod', 'compile', 'complex', 'delattr', 'dict', 'dir', 'divmod',
	'enumerate', 'eval', 'exec', 'filter', 'float', 'format', 'frozenset', 'getattr', 'globals',
	'hasattr', 'hash', 'help', 'hex', 'id', 'input', 'int', 'isinstance', 'issubclass', 'iter', 'len',
	'list', 'locals', 'map', 'max', 'memoryview', 'min', 'next', 'object', 'oct', 'open', 'ord',
	'pow', 'print', 'property', 'range', 'repr', 'reversed', 'round', 'set', 'setattr', 'slice',
	'sorted', 'staticmethod', 'str', 'sum', 'super', 'tuple', 'type', 'vars', 'zip', '__import__'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN .. '.special', {
	'__new__', '__init__', '__del__', '__repr__', '__str__', '__bytes', '__format__', '__lt__',
	'__le__', '__eq__', '__ne__', '__gt__', '__ge__', '__hash__', '__bool__', --
	'__getattr__', '__getattribute__', '__setattr__', '__delattr__', '__dir__', --
	'__get__', '__set__', '__delete__', '__slots__', --
	'__init_subclass__', '__set_name__', --
	'__instancecheck__', '__subclasscheck__', --
	'__class_getitem__', --
	'__call__', --
	'__len__', '__length_hint', '__getitem__', '__setitem__', '__delitem__', '__missing__',
	'__iter__', '__reversed__', '__contains__', --
	'__add__', '__sub__', '__mul__', '__matmul__', '__truediv__', '__floordiv__', '__mod__',
	'__divmod__', '__pow__', '__lshift__', '__rshift__', '__and__', '__xor__', '__or__', --
	'__radd__', '__rsub__', '__rmul__', '__rmatmul__', '__rtruediv__', '__rfloordiv__', '__rmod__',
	'__rdivmod__', '__rpow__', '__rlshift__', '__rrshift__', '__rand__', '__rxor__', '__ror__', --
	'__iadd__', '__isub__', '__imul__', '__imatmul__', '__itruediv__', '__ifloordiv__', '__imod__',
	'__idivmod__', '__ipow__', '__ilshift__', '__irshift__', '__iand__', '__ixor__', '__ior__', --
	'__neg__', '__pos__', '__abs__', '__invert__', '__complex__', '__int__', '__float__', '__index__',
	'__round__', '__trunc__', '__floor__', '__ceil__', --
	'__enter__', '__exit__', --
	'__match_args__', --
	'__await__', --
	'__aiter__', '__anext__', '__aenter__', '__aexit__' --
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'BaseException', 'Exception', 'Exception', 'ArithmeticError', 'BufferError', 'LookupError', --
	'AssertionError', 'AttributeError', 'EOFError', 'FloatingPointError', 'GeneratorExit',
	'ImportError', 'ModuleNotFoundError', 'IndexError', 'KeyError', 'KeyboardInterrupt',
	'MemoryError', 'NameError', 'NotImplementedError', 'OSError', 'OverflowError', 'RecursionError',
	'ReferenceError', 'RuntimeError', 'StopIteration', 'StopAsyncIteration', 'SyntaxError',
	'IndentationError', 'TabError', 'SystemError', 'SystemExit', 'TypeError', 'UnboundLocalError',
	'UnicodeError', 'UnicodeEncodeError', 'UnicodeDecodeError', 'UnicodeTranslateError', 'ValueError',
	'ZeroDivisionError', --
	'EnvironmentError', 'IOError', 'WindowsError', --
	'BlockingIOError', 'ChildProcessError', 'ConnectionError', 'BrokenPipeError',
	'ConnectionAbortedError', 'ConnectionRefusedError', 'FileExistsError', 'FileNotFoundError',
	'InterruptedError', 'IsADirectoryError', 'NotADirectoryError', 'PermissionError',
	'ProcessLookupError', 'TimeoutError', --
	'Warning', 'UserWarning', 'DeprecationWarning', 'PendingDeprecationWarning', 'SyntaxWarning',
	'RuntimeWarning', 'FutureWarning', 'ImportWarning', 'UnicodeWarning', 'BytesWarning',
	'ResourceWarning'
})

lex:set_word_list(lexer.ATTRIBUTE, {
	'__doc__', '__name__', '__qualname__', '__module__', '__defaults__', '__code__', '__globals__',
	'__dict__', '__closure__', '__annotations__', '__kwdefaults__', --
	'__file__', '__bases__', --
	'__class__', --
	'__self__', '__func__' --
})

lexer.property['scintillua.comment'] = '#'

return lex
