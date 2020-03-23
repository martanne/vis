-- Julia lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local B, P, R, S = lpeg.B, lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'julia'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)


-- Identifier
local id = l.word * P('!')^0
local identifier = token(l.IDENTIFIER, id)


-- Operator
local operator = token(l.OPERATOR, S('+-*/÷<>=!≠≈≤≥%^&|⊻~\\\':?.√'))


-- Comment
local line_comment = '#' * l.nonnewline^0
local block_comment = '#=' * (l.any - '=#')^0 * P('=#')^-1
local comment = token(l.COMMENT, block_comment + line_comment)


-- Constant
local const_bool = word_match{'true', 'false'}
local const_numerical = (P('Inf') + P('NaN')) * (P('16') + P('32') + P('64'))^-1 * #(-l.alnum)
local const_special = word_match{'nothing', 'undef', 'missing'}
local const_env = word_match{'ARGS', 'ENV', 'ENDIAN_BOM', 'LOAD_PATH', 'VERSION', 'PROGRAM_FILE', 'DEPOT_PATH'}
local const_io = word_match{'stdout', 'stdin', 'stderr', 'devnull'}
local constant = token(l.CONSTANT, const_bool + const_numerical + const_special + const_env + const_io)


-- Number
local decimal = l.digit^1 * ('_' * l.digit^1)^0
local hex_digits = l.xdigit^1 * ('_' * l.xdigit^1)^0
local hexadecimal = '0x' * hex_digits
local binary = '0b' * S('01')^1 * ('_' * S('01')^1)^0
local integer = binary + hexadecimal + decimal

local float_dec_coeff = decimal^0 * '.' * decimal + decimal * '.' * decimal^0
local float_dec_expon = S('eEf') * S('+-')^-1 * l.digit^1
local float_dec = float_dec_coeff * float_dec_expon^-1 + decimal * float_dec_expon

local float_hex_coeff = '0x' * (hex_digits^0 * '.' * hex_digits + hex_digits * '.' * hex_digits^0)
local float_hex_expon = 'p' * S('+-')^-1 * l.digit^1
local float_hex = float_hex_coeff * float_hex_expon^-1 + hexadecimal * float_hex_expon

local float = float_dec + float_hex

local imaginary = (float_dec + decimal) * 'im'

local number = token(l.NUMBER, S('+-')^-1 * (imaginary + float + integer) * #(-l.alpha))


-- Character & String
local c_esc = '\\' * S('\\"\'nrbtfav')
local unicode = '\\' * S('uU') * l.xdigit^1
local char = "'" * (l.alnum + c_esc + unicode) * "'"
local character = token('character', char)

local doc_str = l.delimited_range('"""')
local str = l.delimited_range('"')
local string = token(l.STRING, doc_str + str)


-- Keyword
local keyword_single = word_match{
  'baremodule', 'begin', 'break', 'catch', 'const', 'continue',
  'do', 'else', 'elseif', 'end', 'export', 'finally', 'for',
  'function', 'global', 'if', 'in', 'isa', 'import', 'let', 'local',
  'macro', 'module', 'quote', 'return', 'struct', 'try', 'using',
  'where', 'while'
}
local keyword_mult = P('abstract type') + P('mutable struct') + P('primitive type')
local keyword = token(l.KEYWORD, keyword_single + keyword_mult)


-- Function
local func = token(l.FUNCTION, id * #(P('.')^-1 * #P('(')))


-- Macro
local macro = token('macro', '@' * (id + '.'))


-- Type
local type_annotated = (B('::') + B(':: ')) * id
local type_para = id * #P('{')
local type_subtyping = id * #(l.space^0 * P('<:')) + (B('<:') + B('<: ')) * id
local type_struct = B('struct ') * id
local type_builtin_numerical = (
    (P('Abstract') + P('Big')) * P('Float') +
    P('Float') * (P('16') + P('32') + P('64')) +
    P('U')^-1 * P('Int') * (P('8') + P('16') + P('32') + P('64') + P('128'))^-1 +
    P('Abstract')^-1 * P('Irrational')
  ) * #(-l.alnum) +
  word_match{'Number', 'Complex', 'Real', 'Integer', 'Bool', 'Signed', 'Unsigned', 'Rational'}
local type_builtin_range = (
    (P('Lin') + P('Ordinal') + (P('Abstract')^-1 * P('Unit')^-1)) * P('Range') +
    P('StepRange') * P('Len')^-1 - P('Range')
  ) * #(-l.alnum)
local type_builtin_array = (
    (P('Abstract') + P('Bit') + P('Dense') + P('PermutedDims') + P('Sub'))^-1 *
    (P('Array') + P('Vector') + P('Matrix') + P('VecOrMat')) +
    (P('Abstract') + P('Sym') + (P('Unit')^-1 * (P('Lower') + P('Upper'))))^-1 * P('Triangular')
  ) * #(-l.alnum) +
  word_match{
    'Adjoint', 'Bidiagonal', 'Diagonal', 'Hermitian', 'LQPackedQ',
    'Symmetric', 'Transpose', 'UpperHessenberg'
  }
local type = token(l.TYPE,
    type_para + type_annotated + type_subtyping + type_struct + type_builtin_numerical + type_builtin_range +
    type_builtin_array
  )


-- Symbol
local symbol = token('symbol', -B(P(':') + P('<')) * P(':') * id)


M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'string', string},
  {'character', character},
  {'keyword', keyword},
  {'constant', constant},
  {'type', type},
  {'symbol', symbol},
  {'macro', macro},
  {'function', func},
  {'identifier', identifier},
  {'operator', operator},
  {'number', number},
}

M._tokenstyles = {
  character = l.STYLE_CONSTANT,
  symbol = l.STYLE_CONSTANT,
  macro = l.STYLE_PREPROCESSOR,
}

return M
