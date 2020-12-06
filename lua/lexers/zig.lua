-- Copyright 2020 Karchnu karchnu@karchnu.fr.
-- Zig LPeg lexer.
-- (Based on the C++ LPeg lexer from Mitchell mitchell.att.foicica.com.)

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'zig'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local comment = token(l.COMMENT, line_comment)
-- For documentation, I took the liberty of using the preprocessor coloration,
-- since it doesn't exist in Zig anyway.
local doc_comment = '///' * l.nonnewline_esc^0
local preprocessor = token(l.PREPROCESSOR, doc_comment)

-- Strings.
local sq_str = P('L')^-1 * l.delimited_range("'", true)
local dq_str = P('L')^-1 * l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{

  -- Keywords.
  'inline', 'pub', 'fn', 'comptime', 'const',
  'extern', 'return', 'var', 'usingnamespace',

  -- Defering code blocks.
  'defer', 'errdefer',

  -- Functions and structures related keywords.
  'align', 'allowzero',
  'noalias', 'noinline',
  'callconv', 'packed', 'linksection',
  'unreachable',
  'test',
  'asm', 'volatile',

  -- Parallelism and concurrency related keywords.
  'async', 'await', 'noasync',
  'suspend', 'nosuspend', 'resume',
  'threadlocal','anyframe',

  -- Control flow: conditions and loops.
  'if', 'else', 'orelse',
  'or', 'and',
  'while', 'for', 'switch', 'continue', 'break',
  'catch', 'try',

  -- Not keyword, but overly used variable name with always the same semantic.
  'self',
})

-- Types.
local type = token(l.TYPE, word_match{
  'enum', 'struct', 'union',
  'i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'i64', 'u64', 'i128', 'u128',
  'isize', 'usize',
  'c_short', 'c_ushort', 'c_int', 'c_uint',
  'c_long', 'c_ulong', 'c_longlong', 'c_ulonglong', 'c_longdouble',
  'c_void',
  'f16', 'f32', 'f64', 'f128',
  'bool',
  'void',
  'noreturn',
  'type', 'anytype', 'error', 'anyerror',
  'comptime_int', 'comptime_float',
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  -- special values
  'false', 'true', 'null', 'undefined',
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Built-in functions.
local functions = token(l.FUNCTION, word_match{
  'addWithOverflow', 'alignCast', 'alignOf', 'as', 'asyncCall', 'atomicLoad', 'atomicRmw',
  'atomicStore', 'bitCast', 'bitOffsetOf', 'boolToInt', 'bitSizeOf', 'breakpoint', 'mulAdd',
  'byteSwap', 'bitReverse', 'byteOffsetOf', 'call', 'cDefine', 'cImport', 'cInclude', 'clz',
  'cmpxchgStrong', 'cmpxchgWeak', 'compileError', 'compileLog', 'ctz', 'cUndef', 'divExact',
  'divFloor', 'divTrunc', 'embedFile', 'enumToInt', 'errorName', 'errorReturnTrace',
  'errorToInt', 'errSetCast', 'export', 'fence', 'field', 'fieldParentPtr', 'floatCast',
  'floatToInt', 'frame', 'Frame', 'frameAddress', 'frameSize', 'hasDecl', 'hasField', 'import',
  'intCast', 'intToEnum', 'intToError', 'intToFloat', 'intToPtr', 'memcpy', 'memset', 'wasmMemorySize',
  'wasmMemoryGrow', 'mod', 'mulWithOverflow', 'panic', 'popCount', 'ptrCast', 'ptrToInt', 'rem',
  'returnAddress', 'setAlignStack', 'setCold', 'setEvalBranchQuota', 'setFloatMode', 'setRuntimeSafety',
  'shlExact', 'shlWithOverflow', 'shrExact', 'shuffle', 'sizeOf', 'splat', 'reduce',
  'src', 'sqrt', 'sin', 'cos', 'exp', 'exp2', 'log', 'log2', 'log10', 'fabs', 'floor',
  'ceil', 'trunc', 'round', 'subWithOverflow', 'tagName', 'TagType', 'This', 'truncate',
  'Type', 'typeInfo', 'typeName', 'TypeOf', 'unionInit',
})


-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'constant', constant},
  {'function', functions},
  {'type', type},
  {'identifier', identifier},
  {'string', string},
  {'preprocessor', preprocessor},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'%l+', '[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['//'] = l.fold_line_comments('//')}
}

return M
