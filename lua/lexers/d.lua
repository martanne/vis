-- Copyright 2006-2025 Mitchell. See LICENSE.
-- D LPeg lexer.
-- Heavily modified by Brian Schott (@Hackerpilot on Github).

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Class names.
local ws = lex:get_rule('whitespace')
lex:add_rule('class',
	lex:tag(lexer.TYPE, P('class') + 'struct') * ws^-1 * lex:tag(lexer.CLASS, lexer.word))

-- Versions.
local open_paren = lex:tag(lexer.OPERATOR, '(')
lex:add_rule('version', lex:tag(lexer.KEYWORD, 'version') * ws^-1 * open_paren * ws^-1 *
	lex:tag(lexer.CONSTANT_BUILTIN .. '.version', lex:word_match('version')))

-- Scopes.
lex:add_rule('scope', lex:tag(lexer.KEYWORD, 'scope') * ws^-1 * open_paren * ws^-1 *
	lex:tag(lexer.CONSTANT_BUILTIN .. '.scope', lexer.word_match('exit success failure')))

-- Traits.
lex:add_rule('trait', lex:tag(lexer.KEYWORD, '__traits') * ws^-1 * open_paren * ws^-1 *
	lex:tag(lexer.VARIABLE_BUILTIN .. '.traits', lex:word_match('trait')))

-- Function names.
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = lpeg.B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (method + func) * #(ws^-1 * ('!' * lexer.word^-1 * ws^-1)^-1 * '('))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Properties.
local dot = lex:tag(lexer.OPERATOR, '.')
lex:add_rule('property', lpeg.B(lexer.alnum + ')') * dot *
	lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match('property')))

-- Strings.
local sq_str = lexer.range("'", true) * S('cwd')^-1
local dq_str = lexer.range('"') * S('cwd')^-1
local lit_str = 'r' * lexer.range('"', false, false) * S('cwd')^-1
local bt_str = lexer.range('`', false, false) * S('cwd')^-1
local hex_str = 'x' * lexer.range('"') * S('cwd')^-1
local other_hex_str = '\\x' * (lexer.xdigit * lexer.xdigit)^1
local str = sq_str + dq_str + lit_str + bt_str + hex_str + other_hex_str
for left, right in pairs{['['] = ']', ['('] = ')', ['{'] = '}', ['<'] = '>'} do
	str = str + lexer.range('q"' .. left, right .. '"', false, false, true) * S('cwd')^-1
end
lex:add_rule('string', lex:tag(lexer.STRING, str))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
local nested_comment = lexer.range('/+', '+/', false, false, true)
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment + nested_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_') * S('uULdDfFi')^-1))

-- Preprocessor.
lex:add_rule('annotation', lex:tag(lexer.ANNOTATION, '@' * lexer.word^1))
lex:add_rule('preprocessor', lex:tag(lexer.PREPROCESSOR, lexer.to_eol('#')))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '..' + S('?=!<>+-*$/%&|^~.,;:()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, '/+', '+/')

-- Word lists.
lex:set_word_list('version', {
	'AArch64', 'AIX', 'all', 'Alpha', 'Alpha_HardFloat', 'Alpha_SoftFloat', 'Android', 'ARM',
	'ARM_HardFloat', 'ARM_SoftFloat', 'ARM_SoftFP', 'ARM_Thumb', 'assert', 'BigEndian', 'BSD',
	'Cygwin', 'D_Coverage', 'D_Ddoc', 'D_HardFloat', 'DigitalMars', 'D_InlineAsm_X86',
	'D_InlineAsm_X86_64', 'D_LP64', 'D_NoBoundsChecks', 'D_PIC', 'DragonFlyBSD', 'D_SIMD',
	'D_SoftFloat', 'D_Version2', 'D_X32', 'FreeBSD', 'GNU', 'Haiku', 'HPPA', 'HPPA64', 'Hurd', 'IA64',
	'LDC', 'linux', 'LittleEndian', 'MIPS32', 'MIPS64', 'MIPS_EABI', 'MIPS_HardFloat', 'MIPS_N32',
	'MIPS_N64', 'MIPS_O32', 'MIPS_O64', 'MIPS_SoftFloat', 'NetBSD', 'none', 'OpenBSD', 'OSX', 'Posix',
	'PPC', 'PPC64', 'PPC_HardFloat', 'PPC_SoftFloat', 'S390', 'S390X', 'SDC', 'SH', 'SH64', 'SkyOS',
	'Solaris', 'SPARC', 'SPARC64', 'SPARC_HardFloat', 'SPARC_SoftFloat', 'SPARC_V8Plus', 'SysV3',
	'SysV4', 'unittest', 'Win32', 'Win64', 'Windows', 'X86', 'X86_64'
})

lex:set_word_list('trait', {
	'allMembers', 'classInstanceSize', 'compiles', 'derivedMembers', 'getAttributes', 'getMember',
	'getOverloads', 'getProtection', 'getUnitTests', 'getVirtualFunctions', 'getVirtualIndex',
	'getVirtualMethods', 'hasMember', 'identifier', 'isAbstractClass', 'isAbstractFunction',
	'isArithmetic', 'isAssociativeArray', 'isFinalClass', 'isFinalFunction', 'isFloating',
	'isIntegral', 'isLazy', 'isNested', 'isOut', 'isOverrideFunction', 'isPOD', 'isRef', 'isSame',
	'isScalar', 'isStaticArray', 'isStaticFunction', 'isUnsigned', 'isVirtualFunction',
	'isVirtualMethod', 'parent'
})

lex:set_word_list(lexer.KEYWORD, {
	'abstract', 'align', 'asm', 'assert', 'auto', 'body', 'break', 'case', 'cast', 'catch', 'const',
	'continue', 'debug', 'default', 'delete', 'deprecated', 'do', 'else', 'extern', 'export', 'false',
	'final', 'finally', 'for', 'foreach', 'foreach_reverse', 'goto', 'if', 'import', 'immutable',
	'in', 'inout', 'invariant', 'is', 'lazy', 'macro', 'mixin', 'new', 'nothrow', 'null', 'out',
	'override', 'pragma', 'private', 'protected', 'public', 'pure', 'ref', 'return', 'scope',
	'shared', 'static', 'super', 'switch', 'synchronized', 'this', 'throwtrue', 'try', 'typeid',
	'typeof', 'unittest', 'version', 'virtual', 'volatile', 'while', 'with', '__gshared', '__thread',
	'__traits', '__vector', '__parameters'
})

lex:set_word_list(lexer.TYPE, {
	'alias', 'bool', 'byte', 'cdouble', 'cent', 'cfloat', 'char', 'class', 'creal', 'dchar',
	'delegate', 'double', 'enum', 'float', 'function', 'idouble', 'ifloat', 'int', 'interface',
	'ireal', 'long', 'module', 'package', 'ptrdiff_t', 'real', 'short', 'size_t', 'struct',
	'template', 'typedef', 'ubyte', 'ucent', 'uint', 'ulong', 'union', 'ushort', 'void', 'wchar',
	'string', 'wstring', 'dstring', 'hash_t', 'equals_t'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'__FILE__', '__LINE__', '__DATE__', '__EOF__', '__TIME__', '__TIMESTAMP__', '__VENDOR__',
	'__VERSION__', '__FUNCTION__', '__PRETTY_FUNCTION__', '__MODULE__'
})

lex:set_word_list('property', {
	'alignof', 'dig', 'dup', 'epsilon', 'idup', 'im', 'init', 'infinity', 'keys', 'length',
	'mangleof', 'mant_dig', 'max', 'max_10_exp', 'max_exp', 'min', 'min_normal', 'min_10_exp',
	'min_exp', 'nan', 'offsetof', 'ptr', 're', 'rehash', 'reverse', 'sizeof', 'sort', 'stringof',
	'tupleof', 'values'
})

lexer.property['scintillua.comment'] = '//'

return lex
