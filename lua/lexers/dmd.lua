-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- D LPeg lexer.
-- Heavily modified by Brian Schott (@Hackerpilot on Github).

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'dmd'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local nested_comment = l.nested_pair('/+', '+/')
local comment = token(l.COMMENT, line_comment + block_comment + nested_comment)

-- Strings.
local sq_str = l.delimited_range("'", true) * S('cwd')^-1
local dq_str = l.delimited_range('"') * S('cwd')^-1
local lit_str = 'r' * l.delimited_range('"', false, true) * S('cwd')^-1
local bt_str = l.delimited_range('`', false, true) * S('cwd')^-1
local hex_str = 'x' * l.delimited_range('"') * S('cwd')^-1
local other_hex_str = '\\x' * (l.xdigit * l.xdigit)^1
local del_str = l.nested_pair('q"[', ']"') * S('cwd')^-1 +
                l.nested_pair('q"(', ')"') * S('cwd')^-1 +
                l.nested_pair('q"{', '}"') * S('cwd')^-1 +
                l.nested_pair('q"<', '>"') * S('cwd')^-1 +
                P('q') * l.nested_pair('{', '}') * S('cwd')^-1
local string = token(l.STRING, del_str + sq_str + dq_str + lit_str + bt_str +
                               hex_str + other_hex_str)

-- Numbers.
local dec = l.digit^1 * ('_' * l.digit^1)^0
local hex_num = l.hex_num * ('_' * l.xdigit^1)^0
local bin_num = '0' * S('bB') * S('01_')^1
local oct_num = '0' * S('01234567_')^1
local integer = S('+-')^-1 * (hex_num + oct_num + bin_num + dec)
local number = token(l.NUMBER, (l.float + integer) * S('uUlLdDfFi')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'abstract', 'align', 'asm', 'assert', 'auto', 'body', 'break', 'case', 'cast',
  'catch', 'const', 'continue', 'debug', 'default', 'delete',
  'deprecated', 'do', 'else', 'extern', 'export', 'false', 'final', 'finally',
  'for', 'foreach', 'foreach_reverse', 'goto', 'if', 'import', 'immutable',
  'in', 'inout', 'invariant', 'is', 'lazy', 'macro', 'mixin', 'new', 'nothrow',
  'null', 'out', 'override', 'pragma', 'private', 'protected', 'public', 'pure',
  'ref', 'return', 'scope', 'shared', 'static', 'super', 'switch',
  'synchronized', 'this', 'throw','true', 'try', 'typeid', 'typeof', 'unittest',
  'version', 'virtual', 'volatile', 'while', 'with', '__gshared', '__thread',
  '__traits', '__vector', '__parameters'
})

-- Types.
local type = token(l.TYPE, word_match{
  'alias', 'bool', 'byte', 'cdouble', 'cent', 'cfloat', 'char', 'class',
  'creal', 'dchar', 'delegate', 'double', 'enum', 'float', 'function',
  'idouble', 'ifloat', 'int', 'interface', 'ireal', 'long', 'module', 'package',
  'ptrdiff_t', 'real', 'short', 'size_t', 'struct', 'template', 'typedef',
  'ubyte', 'ucent', 'uint', 'ulong', 'union', 'ushort', 'void', 'wchar',
  'string', 'wstring', 'dstring', 'hash_t', 'equals_t'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  '__FILE__', '__LINE__', '__DATE__', '__EOF__', '__TIME__', '__TIMESTAMP__',
  '__VENDOR__', '__VERSION__', '__FUNCTION__', '__PRETTY_FUNCTION__',
  '__MODULE__',
})

local class_sequence = token(l.TYPE, P('class') + P('struct')) * ws^1 *
                                     token(l.CLASS, l.word)

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('?=!<>+-*$/%&|^~.,;()[]{}'))

-- Properties.
local properties = (type + identifier + operator) * token(l.OPERATOR, '.') *
  token(l.VARIABLE, word_match{
    'alignof', 'dig', 'dup', 'epsilon', 'idup', 'im', 'init', 'infinity',
    'keys', 'length', 'mangleof', 'mant_dig', 'max', 'max_10_exp', 'max_exp',
    'min', 'min_normal', 'min_10_exp', 'min_exp', 'nan', 'offsetof', 'ptr',
    're', 'rehash', 'reverse', 'sizeof', 'sort', 'stringof', 'tupleof',
    'values'
  })

-- Preprocs.
local annotation = token('annotation', '@' * l.word^1)
local preproc = token(l.PREPROCESSOR, '#' * l.nonnewline^0)

-- Traits.
local traits_list = token('traits', word_match{
  'allMembers', 'classInstanceSize', 'compiles', 'derivedMembers',
  'getAttributes', 'getMember', 'getOverloads', 'getProtection', 'getUnitTests',
  'getVirtualFunctions', 'getVirtualIndex', 'getVirtualMethods', 'hasMember',
  'identifier', 'isAbstractClass', 'isAbstractFunction', 'isArithmetic',
  'isAssociativeArray', 'isFinalClass', 'isFinalFunction', 'isFloating',
  'isIntegral', 'isLazy', 'isNested', 'isOut', 'isOverrideFunction', 'isPOD',
  'isRef', 'isSame', 'isScalar', 'isStaticArray', 'isStaticFunction',
  'isUnsigned', 'isVirtualFunction', 'isVirtualMethod', 'parent'
})

local scopes_list = token('scopes', word_match{'exit', 'success', 'failure'})

-- versions
local versions_list = token('versions', word_match{
  'AArch64', 'AIX', 'all', 'Alpha', 'Alpha_HardFloat', 'Alpha_SoftFloat',
  'Android', 'ARM', 'ARM_HardFloat', 'ARM_SoftFloat', 'ARM_SoftFP', 'ARM_Thumb',
  'assert', 'BigEndian', 'BSD', 'Cygwin', 'D_Coverage', 'D_Ddoc', 'D_HardFloat',
  'DigitalMars', 'D_InlineAsm_X86', 'D_InlineAsm_X86_64', 'D_LP64',
  'D_NoBoundsChecks', 'D_PIC', 'DragonFlyBSD', 'D_SIMD', 'D_SoftFloat',
  'D_Version2', 'D_X32', 'FreeBSD', 'GNU', 'Haiku', 'HPPA', 'HPPA64', 'Hurd',
  'IA64', 'LDC', 'linux', 'LittleEndian', 'MIPS32', 'MIPS64', 'MIPS_EABI',
  'MIPS_HardFloat', 'MIPS_N32', 'MIPS_N64', 'MIPS_O32', 'MIPS_O64',
  'MIPS_SoftFloat', 'NetBSD', 'none', 'OpenBSD', 'OSX', 'Posix', 'PPC', 'PPC64',
  'PPC_HardFloat', 'PPC_SoftFloat', 'S390', 'S390X', 'SDC', 'SH', 'SH64',
  'SkyOS', 'Solaris', 'SPARC', 'SPARC64', 'SPARC_HardFloat', 'SPARC_SoftFloat',
  'SPARC_V8Plus', 'SysV3', 'SysV4', 'unittest', 'Win32', 'Win64', 'Windows',
  'X86', 'X86_64'
})

local versions = token(l.KEYWORD, 'version') * l.space^0 *
                 token(l.OPERATOR, '(') * l.space^0 * versions_list

local scopes = token(l.KEYWORD, 'scope') * l.space^0 *
               token(l.OPERATOR, '(') * l.space^0 * scopes_list

local traits = token(l.KEYWORD, '__traits') * l.space^0 *
               token(l.OPERATOR, '(') * l.space^0 * traits_list

local func = token(l.FUNCTION, l.word) *
             #(l.space^0 * (P('!') * l.word^-1 * l.space^-1)^-1 * P('('))

M._rules = {
  {'whitespace', ws},
  {'class', class_sequence},
  {'traits', traits},
  {'versions', versions},
  {'scopes', scopes},
  {'keyword', keyword},
  {'variable', properties},
  {'type', type},
  {'function', func},
  {'constant', constant},
  {'string', string},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
  {'annotation', annotation},
}

M._tokenstyles = {
  annotation = l.STYLE_PREPROCESSOR,
  traits = l.STYLE_CLASS,
  versions = l.STYLE_CONSTANT,
  scopes = l.STYLE_CONSTANT
}

M._foldsymbols = {
  _patterns = {'[{}]', '/[*+]', '[*+]/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {
    ['/*'] = 1, ['*/'] = -1, ['/+'] = 1, ['+/'] = -1,
    ['//'] = l.fold_line_comments('//')
  }
}

return M
