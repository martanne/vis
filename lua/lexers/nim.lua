-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Nim LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('nim', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
	'addr', 'and', 'as', 'asm', 'atomic', 'bind', 'block', 'break', 'case', 'cast', 'const',
	'continue', 'converter', 'discard', 'distinct', 'div', 'do', 'elif', 'else', 'end', 'enum',
	'except', 'export', 'finally', 'for', 'from', 'generic', 'if', 'import', 'in', 'include',
	'interface', 'is', 'isnot', 'iterator', 'lambda', 'let', 'macro', 'method', 'mixin', 'mod', 'nil',
	'not', 'notin', 'object', 'of', 'or', 'out', 'proc', 'ptr', 'raise', 'ref', 'return', 'shared',
	'shl', 'static', 'template', 'try', 'tuple', 'type', 'var', 'when', 'while', 'with', 'without',
	'xor', 'yield'
}, true)))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match({
	-- Procs.
	'defined', 'definedInScope', 'new', 'unsafeNew', 'internalNew', 'reset', 'high', 'low', 'sizeof',
	'succ', 'pred', 'inc', 'dec', 'newSeq', 'len', 'incl', 'excl', 'card', 'ord', 'chr', 'ze', 'ze64',
	'toU8', 'toU16', 'toU32', 'abs', 'min', 'max', 'contains', 'cmp', 'setLen', 'newString',
	'newStringOfCap', 'add', 'compileOption', 'quit', 'shallowCopy', 'del', 'delete', 'insert',
	'repr', 'toFloat', 'toBiggestFloat', 'toInt', 'toBiggestInt', 'addQuitProc', 'substr', 'zeroMem',
	'copyMem', 'moveMem', 'equalMem', 'swap', 'getRefcount', 'clamp', 'isNil', 'find', 'contains',
	'pop', 'each', 'map', 'GC_ref', 'GC_unref', 'echo', 'debugEcho', 'getTypeInfo', 'Open', 'repopen',
	'Close', 'EndOfFile', 'readChar', 'FlushFile', 'readAll', 'readFile', 'writeFile', 'write',
	'readLine', 'writeln', 'getFileSize', 'ReadBytes', 'ReadChars', 'readBuffer', 'writeBytes',
	'writeChars', 'writeBuffer', 'setFilePos', 'getFilePos', 'fileHandle', 'cstringArrayToSeq',
	'allocCStringArray', 'deallocCStringArray', 'atomicInc', 'atomicDec', 'compareAndSwap',
	'setControlCHook', 'writeStackTrace', 'getStackTrace', 'alloc', 'alloc0', 'dealloc', 'realloc',
	'getFreeMem', 'getTotalMem', 'getOccupiedMem', 'allocShared', 'allocShared0', 'deallocShared',
	'reallocShared', 'IsOnStack', 'GC_addCycleRoot', 'GC_disable', 'GC_enable', 'GC_setStrategy',
	'GC_enableMarkAndSweep', 'GC_disableMarkAndSweep', 'GC_fullCollect', 'GC_getStatistics',
	'nimDestroyRange', 'getCurrentException', 'getCurrentExceptionMsg', 'onRaise', 'likely',
	'unlikely', 'rawProc', 'rawEnv', 'finished', 'slurp', 'staticRead', 'gorge', 'staticExec', 'rand',
	'astToStr', 'InstatiationInfo', 'raiseAssert', 'shallow', 'compiles', 'safeAdd', 'locals',
	-- Iterators.
	'countdown', 'countup', 'items', 'pairs', 'fields', 'fieldPairs', 'lines',
	-- Templates.
	'accumulateResult', 'newException', 'CurrentSourcePath', 'assert', 'doAssert', 'onFailedAssert',
	'eval',
	-- Threads.
	'running', 'joinThread', 'joinThreads', 'createThread', 'threadId', 'myThreadId',
	-- Channels.
	'send', 'recv', 'peek', 'ready'
}, true)))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match({
	'int', 'int8', 'int16', 'int32', 'int64', 'uint', 'uint8', 'uint16', 'uint32', 'uint64', 'float',
	'float32', 'float64', 'bool', 'char', 'string', 'cstring', 'pointer', 'Ordinal', 'auto', 'any',
	'TSignedInt', 'TUnsignedInt', 'TInteger', 'TOrdinal', 'TReal', 'TNumber', 'range', 'array',
	'openarray', 'varargs', 'seq', 'set', 'TSlice', 'TThread', 'TChannel',
	-- Meta Types.
	'expr', 'stmt', 'typeDesc', 'void'
}, true)))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
	'on', 'off', 'isMainModule', 'CompileDate', 'CompileTime', 'NimVersion', 'NimMajor', 'NimMinor',
	'NimPatch', 'cpuEndian', 'hostOS', 'hostCPU', 'appType', 'QuitSuccess', 'QuitFailure', 'inf',
	'neginf', 'nan'
}))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local tq_str = lexer.range('"""')
local raw_str = 'r' * lexer.range('"', false, false)
lex:add_rule('string', token(lexer.STRING, tq_str + sq_str + dq_str + raw_str))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('#', true)
local block_comment = lexer.range('#[', ']#')
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.float_('_') + lexer.integer_('_') *
	("'" * S('iIuUfF') * (P('8') + '16' + '32' + '64'))^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=+-*/<>@$~&%|!?^.:\\`()[]{},;')))

lexer.property['scintillua.comment'] = '#'

return lex
