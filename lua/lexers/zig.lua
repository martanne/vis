-- Copyright 2020-2022 Karchnu karchnu@karchnu.fr. See LICENSE.
-- Zig LPeg lexer.
-- (Based on the C++ LPeg lexer from Mitchell.)

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('zig')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- Keywords.
  'inline', 'pub', 'fn', 'comptime', 'const', 'extern', 'return', 'var', 'usingnamespace',
  -- Defering code blocks.
  'defer', 'errdefer',
  -- Functions and structures related keywords.
  'align', 'allowzero', 'noalias', 'noinline', 'callconv', 'packed', 'linksection', 'unreachable',
  'test', 'asm', 'volatile',
  -- Parallelism and concurrency related keywords.
  'async', 'await', 'noasync', 'suspend', 'nosuspend', 'resume', 'threadlocalanyframe',
  -- Control flow: conditions and loops.
  'if', 'else', 'orelse', 'or', 'and', 'while', 'for', 'switch', 'continue', 'break', 'catch',
  'try',
  -- Not keyword but overly used variable name with always the same semantic.
  'self'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'enum', 'struct', 'union', --
  'i8', 'u8', 'i16', 'u16', 'i32', 'u32', 'i64', 'u64', 'i128', 'u128', --
  'isize', 'usize', --
  'c_short', 'c_ushort', 'c_int', 'c_uint', --
  'c_long', 'c_ulong', 'c_longlong', 'c_ulonglong', 'c_longdouble', --
  'c_void', --
  'f16', 'f32', 'f64', 'f128', --
  'bool', 'void', 'noreturn', 'type', 'anytype', 'error', 'anyerror', --
  'comptime_int', 'comptime_float'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
  -- Special values.
  'false', 'true', 'null', 'undefined'
}))

-- Built-in functions.
lex:add_rule('function', token(lexer.FUNCTION, '@' * word_match{
  'addWithOverflow', 'alignCast', 'alignOf', 'as', 'asyncCall', 'atomicLoad', 'atomicRmw',
  'atomicStore', 'bitCast', 'bitOffsetOf', 'boolToInt', 'bitSizeOf', 'breakpoint', 'mulAdd',
  'byteSwap', 'bitReverse', 'byteOffsetOf', 'call', 'cDefine', 'cImport', 'cInclude', 'clz',
  'cmpxchgStrong', 'cmpxchgWeak', 'compileError', 'compileLog', 'ctz', 'cUndef', 'divExact',
  'divFloor', 'divTrunc', 'embedFile', 'enumToInt', 'errorName', 'errorReturnTrace', 'errorToInt',
  'errSetCast', 'export', 'fence', 'field', 'fieldParentPtr', 'floatCast', 'floatToInt', 'frame',
  'Frame', 'frameAddress', 'frameSize', 'hasDecl', 'hasField', 'import', 'intCast', 'intToEnum',
  'intToError', 'intToFloat', 'intToPtr', 'memcpy', 'memset', 'wasmMemorySize', 'wasmMemoryGrow',
  'mod', 'mulWithOverflow', 'panic', 'popCount', 'ptrCast', 'ptrToInt', 'rem', 'returnAddress',
  'setAlignStack', 'setCold', 'setEvalBranchQuota', 'setFloatMode', 'setRuntimeSafety', 'shlExact',
  'shlWithOverflow', 'shrExact', 'shuffle', 'sizeOf', 'splat', 'reduce', 'src', 'sqrt', 'sin',
  'cos', 'exp', 'exp2', 'log', 'log2', 'log10', 'fabs', 'floor', 'ceil', 'trunc', 'round',
  'subWithOverflow', 'tagName', 'TagType', 'This', 'truncate', 'Type', 'typeInfo', 'typeName',
  'TypeOf', 'unionInit'
}))

-- Strings.
local sq_str = P('L')^-1 * lexer.range("'", true)
local dq_str = P('L')^-1 * lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
lex:add_rule('doc_comment', token('doc_comment', lexer.to_eol('///', true)))
lex:add_style('doc_comment', lexer.styles.comment)
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('//', true)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))
lex:add_fold_point(lexer.PREPROCESSOR, lexer.fold_consecutive_lines('///'))

return lex
