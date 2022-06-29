-- Copyright 2020-2022 Tobias Frilling. See LICENSE.
-- Julia lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local B, P, S = lpeg.B, lpeg.P, lpeg.S

local lex = lexer.new('julia')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

local id = lexer.word * P('!')^0

-- Keyword
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'baremodule', 'begin', 'break', 'catch', 'const', 'continue', 'do', 'else', 'elseif', 'end',
  'export', 'finally', 'for', 'function', 'global', 'if', 'in', 'isa', 'import', 'let', 'local',
  'macro', 'module', 'quote', 'return', 'struct', 'try', 'using', 'where', 'while'
} + 'abstract type' + 'mutable struct' + 'primitive type'))

-- Constant
local const_bool = word_match('true false')
local const_numerical = (P('Inf') + 'NaN') * (P('16') + '32' + '64')^-1 * -lexer.alnum
local const_special = word_match('nothing undef missing')
local const_env = word_match('ARGS ENV ENDIAN_BOM LOAD_PATH VERSION PROGRAM_FILE DEPOT_PATH')
local const_io = word_match('stdout stdin stderr devnull')
lex:add_rule('constant', token(lexer.CONSTANT,
  const_bool + const_numerical + const_special + const_env + const_io))

-- Type
local type_annotated = (B('::') + B(':: ')) * id
local type_para = id * #P('{')
local type_subtyping = id * #(lexer.space^0 * '<:') + (B('<:') + B('<: ')) * id
local type_struct = B('struct ') * id
-- LuaFormatter off
local type_builtin_numerical = ((P('Abstract') + 'Big') * 'Float' +
  'Float' * (P('16') + '32' + '64') +
  P('U')^-1 * 'Int' * (P('8') + '16' + '32' + '64' + '128')^-1 +
  P('Abstract')^-1 * 'Irrational'
) * -lexer.alnum + word_match('Number Complex Real Integer Bool Signed Unsigned Rational')
-- LuaFormatter on
local type_builtin_range = ((P('Lin') + 'Ordinal' + (P('Abstract')^-1 * P('Unit')^-1)) * 'Range' +
  'StepRange' * P('Len')^-1 - 'Range'
) * -lexer.alnum
local type_builtin_array = ((P('Abstract') + 'Bit' + 'Dense' + 'PermutedDims' + 'Sub')^-1 *
  word_match('Array Vector Matrix VecOrMat') +
  (P('Abstract') + 'Sym' + (P('Unit')^-1 * (P('Lower') + 'Upper')))^-1 * 'Triangular'
) * -lexer.alnum +
  word_match('Adjoint Bidiagonal Diagonal Hermitian LQPackedQ Symmetric Transpose UpperHessenberg')
lex:add_rule('type', token(lexer.TYPE,
  type_para + type_annotated + type_subtyping + type_struct + type_builtin_numerical +
    type_builtin_range + type_builtin_array))

-- Macro
lex:add_rule('macro', token('macro', '@' * (id + '.')))
lex:add_style('macro', lexer.styles.preprocessor)

-- Symbol
lex:add_rule('symbol', token('symbol', -B(P(':') + '<') * ':' * id))
lex:add_style('symbol', lexer.styles.constant)

-- Function
lex:add_rule('function', token(lexer.FUNCTION, id * #(P('.')^-1 * '(')))

-- Identifier
lex:add_rule('identifier', token(lexer.IDENTIFIER, id))

-- Comment
local line_comment = lexer.to_eol('#')
local block_comment = lexer.range('#=', '=#')
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Number
local decimal = lexer.digit^1 * ('_' * lexer.digit^1)^0
local hex_digits = lexer.xdigit^1 * ('_' * lexer.xdigit^1)^0
local hexadecimal = '0x' * hex_digits
local binary = '0b' * S('01')^1 * ('_' * S('01')^1)^0
local integer = binary + hexadecimal + decimal

local float_dec_coeff = decimal^0 * '.' * decimal + decimal * '.' * decimal^0
local float_dec_expon = S('eEf') * S('+-')^-1 * lexer.digit^1
local float_dec = float_dec_coeff * float_dec_expon^-1 + decimal * float_dec_expon

local float_hex_coeff = '0x' * (hex_digits^0 * '.' * hex_digits + hex_digits * '.' * hex_digits^0)
local float_hex_expon = 'p' * S('+-')^-1 * lexer.digit^1
local float_hex = float_hex_coeff * float_hex_expon^-1 + hexadecimal * float_hex_expon

local float = float_dec + float_hex

local imaginary = (float_dec + decimal) * 'im'

lex:add_rule('number',
  token(lexer.NUMBER, S('+-')^-1 * (imaginary + float + integer) * -lexer.alpha))

-- String & Character
local doc_str = lexer.range('"""')
local str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, doc_str + str))

local c_esc = '\\' * S('\\"\'nrbtfav')
local unicode = '\\' * S('uU') * lexer.xdigit^1
local char = "'" * (lexer.alnum + c_esc + unicode) * "'"
lex:add_rule('character', token('character', char))
lex:add_style('character', lexer.styles.constant)

-- Operator
lex:add_rule('operator', token(lexer.OPERATOR, S('+-*/÷<>=!≠≈≤≥%^&|⊻~\\\':?.√')))

return lex
