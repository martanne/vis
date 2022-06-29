-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Python LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('python', {fold_by_indentation = true})

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Classes.
lex:add_rule('classdef', token(lexer.KEYWORD, 'class') * ws * token(lexer.CLASS, lexer.word))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'and', 'as', 'assert', 'async', 'await', 'break', 'continue', 'def', 'del', 'elif', 'else',
  'except', 'exec', 'finally', 'for', 'from', 'global', 'if', 'import', 'in', 'is', 'lambda',
  'nonlocal', 'not', 'or', 'pass', 'print', 'raise', 'return', 'try', 'while', 'with', 'yield',
  -- Descriptors/attr access.
  '__get__', '__set__', '__delete__', '__slots__',
  -- Class.
  '__new__', '__init__', '__del__', '__repr__', '__str__', '__cmp__', '__index__', '__lt__',
  '__le__', '__gt__', '__ge__', '__eq__', '__ne__', '__hash__', '__nonzero__', '__getattr__',
  '__getattribute__', '__setattr__', '__delattr__', '__call__',
  -- Operator.
  '__add__', '__sub__', '__mul__', '__div__', '__floordiv__', '__mod__', '__divmod__', '__pow__',
  '__and__', '__xor__', '__or__', '__lshift__', '__rshift__', '__nonzero__', '__neg__', '__pos__',
  '__abs__', '__invert__', '__iadd__', '__isub__', '__imul__', '__idiv__', '__ifloordiv__',
  '__imod__', '__ipow__', '__iand__', '__ixor__', '__ior__', '__ilshift__', '__irshift__',
  -- Conversions.
  '__int__', '__long__', '__float__', '__complex__', '__oct__', '__hex__', '__coerce__',
  -- Containers.
  '__len__', '__getitem__', '__missing__', '__setitem__', '__delitem__', '__contains__', '__iter__',
  '__getslice__', '__setslice__', '__delslice__',
  -- Module and class attribs.
  '__doc__', '__name__', '__dict__', '__file__', '__path__', '__module__', '__bases__', '__class__',
  '__self__',
  -- Stdlib/sys.
  '__builtin__', '__future__', '__main__', '__import__', '__stdin__', '__stdout__', '__stderr__',
  -- Other.
  '__debug__', '__doc__', '__import__', '__name__'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'abs', 'all', 'any', 'apply', 'basestring', 'bool', 'buffer', 'callable', 'chr', 'classmethod',
  'cmp', 'coerce', 'compile', 'complex', 'copyright', 'credits', 'delattr', 'dict', 'dir', 'divmod',
  'enumerate', 'eval', 'execfile', 'exit', 'file', 'filter', 'float', 'frozenset', 'getattr',
  'globals', 'hasattr', 'hash', 'help', 'hex', 'id', 'input', 'int', 'intern', 'isinstance',
  'issubclass', 'iter', 'len', 'license', 'list', 'locals', 'long', 'map', 'max', 'min', 'object',
  'oct', 'open', 'ord', 'pow', 'property', 'quit', 'range', 'raw_input', 'reduce', 'reload', 'repr',
  'reversed', 'round', 'set', 'setattr', 'slice', 'sorted', 'staticmethod', 'str', 'sum', 'super',
  'tuple', 'type', 'unichr', 'unicode', 'vars', 'xrange', 'zip'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
  'ArithmeticError', 'AssertionError', 'AttributeError', 'BaseException', 'DeprecationWarning',
  'EOFError', 'Ellipsis', 'EnvironmentError', 'Exception', 'False', 'FloatingPointError',
  'FutureWarning', 'GeneratorExit', 'IOError', 'ImportError', 'ImportWarning', 'IndentationError',
  'IndexError', 'KeyError', 'KeyboardInterrupt', 'LookupError', 'MemoryError', 'NameError', 'None',
  'NotImplemented', 'NotImplementedError', 'OSError', 'OverflowError', 'PendingDeprecationWarning',
  'ReferenceError', 'RuntimeError', 'RuntimeWarning', 'StandardError', 'StopIteration',
  'SyntaxError', 'SyntaxWarning', 'SystemError', 'SystemExit', 'TabError', 'True', 'TypeError',
  'UnboundLocalError', 'UnicodeDecodeError', 'UnicodeEncodeError', 'UnicodeError',
  'UnicodeTranslateError', 'UnicodeWarning', 'UserWarning', 'ValueError', 'Warning',
  'ZeroDivisionError'
}))

-- Self.
lex:add_rule('self', token('self', 'self'))
lex:add_style('self', lexer.styles.type)

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#', true)))

-- Strings.
local sq_str = P('u')^-1 * lexer.range("'", true)
local dq_str = P('U')^-1 * lexer.range('"', true)
local tq_str = lexer.range("'''") + lexer.range('"""')
-- TODO: raw_strs cannot end in single \.
local raw_sq_str = P('u')^-1 * 'r' * lexer.range("'", false, false)
local raw_dq_str = P('U')^-1 * 'R' * lexer.range('"', false, false)
lex:add_rule('string', token(lexer.STRING, tq_str + sq_str + dq_str + raw_sq_str + raw_dq_str))

-- Numbers.
local dec = lexer.dec_num * S('Ll')^-1
local bin = '0b' * S('01')^1 * ('_' * S('01')^1)^0
local oct = lexer.oct_num * S('Ll')^-1
local integer = S('+-')^-1 * (bin + lexer.hex_num + oct + dec)
lex:add_rule('number', token(lexer.NUMBER, lexer.float + integer))

-- Decorators.
lex:add_rule('decorator', token('decorator', lexer.to_eol('@')))
lex:add_style('decorator', lexer.styles.preprocessor)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!%^&*()[]{}-=+/|:;.,?<>~`')))

return lex
