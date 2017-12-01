-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Python LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'python'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline_esc^0)

-- Strings.
local sq_str = P('u')^-1 * l.delimited_range("'", true)
local dq_str = P('U')^-1 * l.delimited_range('"', true)
local triple_sq_str = "'''" * (l.any - "'''")^0 * P("'''")^-1
local triple_dq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
-- TODO: raw_strs cannot end in single \.
local raw_sq_str = P('u')^-1 * 'r' * l.delimited_range("'", false, true)
local raw_dq_str = P('U')^-1 * 'R' * l.delimited_range('"', false, true)
local string = token(l.STRING, triple_sq_str + triple_dq_str + sq_str + dq_str +
                               raw_sq_str + raw_dq_str)

-- Numbers.
local dec = l.digit^1 * S('Ll')^-1
local bin = '0b' * S('01')^1 * ('_' * S('01')^1)^0
local oct = '0' * R('07')^1 * S('Ll')^-1
local integer = S('+-')^-1 * (bin + l.hex_num + oct + dec)
local number = token(l.NUMBER, l.float + integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'async', 'await',
  'and', 'as', 'assert', 'break', 'class', 'continue', 'def', 'del', 'elif',
  'else', 'except', 'exec', 'finally', 'for', 'from', 'global', 'if', 'import',
  'in', 'is', 'lambda', 'nonlocal', 'not', 'or', 'pass', 'print', 'raise',
  'return', 'try', 'while', 'with', 'yield',
  -- Descriptors/attr access.
  '__get__', '__set__', '__delete__', '__slots__',
  -- Class.
  '__new__', '__init__', '__del__', '__repr__', '__str__', '__cmp__',
  '__index__', '__lt__', '__le__', '__gt__', '__ge__', '__eq__', '__ne__',
  '__hash__', '__nonzero__', '__getattr__', '__getattribute__', '__setattr__',
  '__delattr__', '__call__',
  -- Operator.
  '__add__', '__sub__', '__mul__', '__div__', '__floordiv__', '__mod__',
  '__divmod__', '__pow__', '__and__', '__xor__', '__or__', '__lshift__',
  '__rshift__', '__nonzero__', '__neg__', '__pos__', '__abs__', '__invert__',
  '__iadd__', '__isub__', '__imul__', '__idiv__', '__ifloordiv__', '__imod__',
  '__ipow__', '__iand__', '__ixor__', '__ior__', '__ilshift__', '__irshift__',
  -- Conversions.
  '__int__', '__long__', '__float__', '__complex__', '__oct__', '__hex__',
  '__coerce__',
  -- Containers.
  '__len__', '__getitem__', '__missing__', '__setitem__', '__delitem__',
  '__contains__', '__iter__', '__getslice__', '__setslice__', '__delslice__',
  -- Module and class attribs.
  '__doc__', '__name__', '__dict__', '__file__', '__path__', '__module__',
  '__bases__', '__class__', '__self__',
  -- Stdlib/sys.
  '__builtin__', '__future__', '__main__', '__import__', '__stdin__',
  '__stdout__', '__stderr__',
  -- Other.
  '__debug__', '__doc__', '__import__', '__name__'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'abs', 'all', 'any', 'apply', 'basestring', 'bool', 'buffer', 'callable',
  'chr', 'classmethod', 'cmp', 'coerce', 'compile', 'complex', 'copyright',
  'credits', 'delattr', 'dict', 'dir', 'divmod', 'enumerate', 'eval',
  'execfile', 'exit', 'file', 'filter', 'float', 'frozenset', 'getattr',
  'globals', 'hasattr', 'hash', 'help', 'hex', 'id', 'input', 'int', 'intern',
  'isinstance', 'issubclass', 'iter', 'len', 'license', 'list', 'locals',
  'long', 'map', 'max', 'min', 'object', 'oct', 'open', 'ord', 'pow',
  'property', 'quit', 'range', 'raw_input', 'reduce', 'reload', 'repr',
  'reversed', 'round', 'set', 'setattr', 'slice', 'sorted', 'staticmethod',
  'str', 'sum', 'super', 'tuple', 'type', 'unichr', 'unicode', 'vars', 'xrange',
  'zip'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'ArithmeticError', 'AssertionError', 'AttributeError', 'BaseException',
  'DeprecationWarning', 'EOFError', 'Ellipsis', 'EnvironmentError', 'Exception',
  'False', 'FloatingPointError', 'FutureWarning', 'GeneratorExit', 'IOError',
  'ImportError', 'ImportWarning', 'IndentationError', 'IndexError', 'KeyError',
  'KeyboardInterrupt', 'LookupError', 'MemoryError', 'NameError', 'None',
  'NotImplemented', 'NotImplementedError', 'OSError', 'OverflowError',
  'PendingDeprecationWarning', 'ReferenceError', 'RuntimeError',
  'RuntimeWarning', 'StandardError', 'StopIteration', 'SyntaxError',
  'SyntaxWarning', 'SystemError', 'SystemExit', 'TabError', 'True', 'TypeError',
  'UnboundLocalError', 'UnicodeDecodeError', 'UnicodeEncodeError',
  'UnicodeError', 'UnicodeTranslateError', 'UnicodeWarning', 'UserWarning',
  'ValueError', 'Warning', 'ZeroDivisionError'
})

-- Self.
local self = token('self', P('self'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('!%^&*()[]{}-=+/|:;.,?<>~`'))

-- Decorators.
local decorator = token('decorator', l.starts_line('@') * l.nonnewline^0)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'constant', constant},
  {'self', self},
  {'identifier', identifier},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'decorator', decorator},
  {'operator', operator},
}


M._tokenstyles = {
  self = l.STYLE_TYPE,
  decorator = l.STYLE_PREPROCESSOR
}

M._FOLDBYINDENTATION = true

return M
