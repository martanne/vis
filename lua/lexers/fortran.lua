-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Fortran LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('fortran')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
local line_comment = lexer.to_eol(lexer.starts_line(S('CcDd!*')) + '!')
lex:add_rule('comment', token(lexer.COMMENT, line_comment))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'include', 'interface', 'program', 'module', 'subroutine', 'function', 'contains', 'use', 'call',
  'return',
  -- Statements.
  'case', 'select', 'default', 'continue', 'cycle', 'do', 'while', 'else', 'if', 'elseif', 'then',
  'elsewhere', 'end', 'endif', 'enddo', 'equivalence', 'external', 'forall', 'where', 'exit',
  'goto', 'pause', 'save', 'stop',
  -- Operators.
  '.not.', '.and.', '.or.', '.xor.', '.eqv.', '.neqv.', '.eq.', '.ne.', '.gt.', '.ge.', '.lt.',
  '.le.',
  -- Logical.
  '.false.', '.true.',
  -- Attributes and other keywords.
  'access', 'action', 'advance', 'assignment', 'block', 'entry', 'in', 'inout', 'intent', 'only',
  'out', 'optional', 'pointer', 'precision', 'procedure', 'recursive', 'result', 'sequence', 'size',
  'stat', 'target', 'type'
}, true)))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match({
  -- I/O.
  'backspace', 'close', 'endfile', 'inquire', 'open', 'print', 'read', 'rewind', 'write', 'format',
  -- Type conversion utility and math.
  'aimag', 'aint', 'amax0', 'amin0', 'anint', 'ceiling', 'cmplx', 'conjg', 'dble', 'dcmplx',
  'dfloat', 'dim', 'dprod', 'float', 'floor', 'ifix', 'imag', 'int', 'logical', 'modulo', 'nint',
  'real', 'sign', 'sngl', 'transfer', 'zext', 'abs', 'acos', 'aimag', 'aint', 'alog', 'alog10',
  'amax0', 'amax1', 'amin0', 'amin1', 'amod', 'anint', 'asin', 'atan', 'atan2', 'cabs', 'ccos',
  'char', 'clog', 'cmplx', 'conjg', 'cos', 'cosh', 'csin', 'csqrt', 'dabs', 'dacos', 'dasin',
  'datan', 'datan2', 'dble', 'dcos', 'dcosh', 'ddim', 'dexp', 'dim', 'dint', 'dlog', 'dlog10',
  'dmax1', 'dmin1', 'dmod', 'dnint', 'dprod', 'dreal', 'dsign', 'dsin', 'dsinh', 'dsqrt', 'dtan',
  'dtanh', 'exp', 'float', 'iabs', 'ichar', 'idim', 'idint', 'idnint', 'ifix', 'index', 'int',
  'isign', 'len', 'lge', 'lgt', 'lle', 'llt', 'log', 'log10', 'max', 'max0', 'max1', 'min', 'min0',
  'min1', 'mod', 'nint', 'real', 'sign', 'sin', 'sinh', 'sngl', 'sqrt', 'tan', 'tanh',
  -- Matrix math.
  'matmul', 'transpose', 'reshape',
  -- Other frequently used built-in statements.
  'assign', 'nullify',
  -- ISO C binding from Fortran 2003.
  'c_sizeof', 'c_f_pointer', 'c_associated'
}, true)))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match({
  'implicit', 'explicit', 'none', 'data', 'parameter', 'allocate', 'allocatable', 'allocated',
  'deallocate', 'integer', 'real', 'double', 'precision', 'complex', 'logical', 'character',
  'dimension', 'kind',
  -- ISO C binding from Fortran 2003
  'bind', 'c_int', 'c_short', 'c_long', 'c_long_long', 'c_signed_char', 'c_size_t', 'c_int8_t',
  'c_int16_t', 'c_int32_t', 'c_int64_t', 'c_int128_t', 'c_intptr_t', 'c_float', 'c_double',
  'c_long_double', 'c_float128', 'c_float_complex', 'c_double_complex', 'c_long_double_complex',
  'c_float128_complex', 'c_bool', 'c_char', 'c_null_char', 'c_new_line', 'c_null_ptr', 'c_funptr'
}, true)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * -lexer.alpha))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.alnum^1))

-- Strings.
local sq_str = lexer.range("'", true, false)
local dq_str = lexer.range('"', true, false)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('<>=&+-/*,()')))

return lex
