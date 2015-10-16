-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Nim LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'nim'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline_esc^0)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local triple_dq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local raw_dq_str = 'r' * l.delimited_range('"', false, true)
local string = token(l.STRING, triple_dq_str + sq_str + dq_str + raw_dq_str)

-- Numbers.
local dec = l.digit^1 * ('_' * l.digit^1)^0
local hex = '0' * S('xX') * l.xdigit^1 * ('_' * l.xdigit^1)^0
local bin = '0' * S('bB') * S('01')^1 * ('_' * S('01')^1)^0
local oct = '0o' * R('07')^1
local integer = S('+-')^-1 * (bin + hex + oct + dec) *
                ("'" * S('iIuUfF') * (P('8') + '16' + '32' + '64'))^-1
local float = l.digit^1 * ('_' * l.digit^1)^0 * ('.' * ('_' * l.digit)^0)^-1 *
              S('eE') * S('+-')^-1 * l.digit^1 * ('_' * l.digit^1)^0
local number = token(l.NUMBER, l.float + integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'addr', 'and', 'as', 'asm', 'atomic', 'bind', 'block', 'break', 'case',
  'cast', 'const', 'continue', 'converter', 'discard', 'distinct', 'div', 'do',
  'elif', 'else', 'end', 'enum', 'except', 'export', 'finally', 'for', 'from',
  'generic', 'if', 'import', 'in', 'include', 'interface', 'is', 'isnot',
  'iterator', 'lambda', 'let', 'macro', 'method', 'mixin', 'mod', 'nil', 'not',
  'notin', 'object', 'of', 'or', 'out', 'proc', 'ptr', 'raise', 'ref', 'return',
  'shared', 'shl', 'static', 'template', 'try', 'tuple', 'type', 'var', 'when',
  'while', 'with', 'without', 'xor', 'yield'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, word_match({
  -- Procs.
  'defined', 'definedInScope', 'new', 'unsafeNew', 'internalNew', 'reset',
  'high', 'low', 'sizeof', 'succ', 'pred', 'inc', 'dec', 'newSeq', 'len',
  'incl', 'excl', 'card', 'ord', 'chr', 'ze', 'ze64', 'toU8', 'toU16', 'toU32',
  'abs', 'min', 'max', 'contains', 'cmp', 'setLen', 'newString',
  'newStringOfCap', 'add', 'compileOption', 'quit', 'shallowCopy', 'del',
  'delete', 'insert', 'repr', 'toFloat', 'toBiggestFloat', 'toInt',
  'toBiggestInt', 'addQuitProc', 'substr', 'zeroMem', 'copyMem', 'moveMem',
  'equalMem', 'swap', 'getRefcount', 'clamp', 'isNil', 'find',   'contains',
  'pop', 'each', 'map', 'GC_ref', 'GC_unref', 'echo', 'debugEcho',
  'getTypeInfo', 'Open', 'repopen', 'Close', 'EndOfFile', 'readChar',
  'FlushFile', 'readAll', 'readFile', 'writeFile', 'write', 'readLine',
  'writeln', 'getFileSize', 'ReadBytes', 'ReadChars', 'readBuffer',
  'writeBytes', 'writeChars', 'writeBuffer', 'setFilePos', 'getFilePos',
  'fileHandle', 'cstringArrayToSeq', 'allocCStringArray', 'deallocCStringArray',
  'atomicInc', 'atomicDec', 'compareAndSwap', 'setControlCHook',
  'writeStackTrace', 'getStackTrace', 'alloc', 'alloc0', 'dealloc', 'realloc',
  'getFreeMem', 'getTotalMem', 'getOccupiedMem', 'allocShared', 'allocShared0',
  'deallocShared', 'reallocShared', 'IsOnStack', 'GC_addCycleRoot',
  'GC_disable', 'GC_enable', 'GC_setStrategy', 'GC_enableMarkAndSweep',
  'GC_disableMarkAndSweep', 'GC_fullCollect', 'GC_getStatistics',
  'nimDestroyRange', 'getCurrentException', 'getCurrentExceptionMsg', 'onRaise',
  'likely', 'unlikely', 'rawProc', 'rawEnv', 'finished', 'slurp', 'staticRead',
  'gorge', 'staticExec', 'rand', 'astToStr', 'InstatiationInfo', 'raiseAssert',
  'shallow', 'compiles', 'safeAdd', 'locals',
  -- Iterators.
  'countdown', 'countup', 'items', 'pairs', 'fields', 'fieldPairs', 'lines',
  -- Templates.
  'accumulateResult', 'newException', 'CurrentSourcePath', 'assert', 'doAssert',
  'onFailedAssert', 'eval',
  -- Threads.
  'running', 'joinThread', 'joinThreads', 'createThread', 'threadId',
  'myThreadId',
  -- Channels.
  'send', 'recv', 'peek', 'ready'
}, nil, true))

-- Types.
local type = token(l.TYPE , word_match({
  'int', 'int8', 'int16', 'int32', 'int64', 'uint', 'uint8', 'uint16', 'uint32',
  'uint64', 'float', 'float32', 'float64', 'bool', 'char', 'string', 'cstring',
  'pointer', 'Ordinal', 'auto', 'any', 'TSignedInt', 'TUnsignedInt', 'TInteger',
  'TOrdinal', 'TReal', 'TNumber', 'range', 'array', 'openarray', 'varargs',
  'seq', 'set', 'TSlice', 'TThread', 'TChannel',
  -- Meta Types.
  'expr', 'stmt', 'typeDesc', 'void',
}, nil, true))

-- Constants.
local constant = token(l.CONSTANT, word_match{
  'on', 'off', 'isMainModule', 'CompileDate', 'CompileTime', 'NimVersion',
  'NimMajor', 'NimMinor', 'NimPatch', 'cpuEndian', 'hostOS', 'hostCPU',
  'appType', 'QuitSuccess', 'QuitFailure', 'inf', 'neginf', 'nan'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('=+-*/<>@$~&%|!?^.:\\`()[]{},;'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'type', type},
  {'constant', constant},
  {'identifier', identifier},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'operator', operator},
}

M._FOLDBYINDENTATION = true

return M
