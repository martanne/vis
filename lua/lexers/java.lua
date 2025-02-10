-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Java LPeg lexer.
-- Modified by Brian Schott.

local lexer = require('lexer')
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Classes.
lex:add_rule('classdef', lex:tag(lexer.KEYWORD, 'class') * lex:get_rule('whitespace') *
	lex:tag(lexer.CLASS, lexer.word))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
local builtin_func = lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = lpeg.B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN) +
	'System.' * lexer.word_match('err in out')))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number * S('LlFfDd')^-1))

-- Annotations.
lex:add_rule('annotation', lex:tag(lexer.ANNOTATION, '@' * lexer.word))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'abstract', 'assert', 'break', 'case', 'catch', 'class', 'const', 'continue', 'default', 'do',
	'else', 'enum', 'extends', 'final', 'finally', 'for', 'goto', 'if', 'implements', 'import',
	'instanceof', 'interface', 'native', 'new', 'package', 'private', 'protected', 'public', 'return',
	'static', 'strictfp', 'super', 'switch', 'synchronized', 'this', 'throw', 'throws', 'transient',
	'try', 'while', 'volatile', --
	'true', 'false', 'null' -- literals
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'clone', 'equals', 'finalize', 'getClass', 'hashCode', 'notify', 'notifyAll', 'toString', 'wait', --
	'Boolean.compare', 'Boolean.getBoolean', 'Boolean.parseBoolean', 'Boolean.valueOf', --
	'Byte.compare', 'Byte.decode', 'Byte.parseByte', 'Byte.valueOf', --
	'Character.charCount', 'Character.codePointAt', 'Character.codePointBefore',
	'Character.codePointCount', 'Character.compare', 'Character.digit', 'Character.forDigit',
	'Character.getName', 'Character.getNumericValue', 'Character.getType', 'Character.isAlphabetic',
	'Character.isDefined', 'Character.isDigit', 'Character.isIdentifierIgnorable',
	'Character.isIdeographic', 'Character.isISOControl', 'Character.isJavaIdentifierPart',
	'Character.isJavaIdentifierStart', 'Character.isLetter', 'Character.isLetterOrDigit',
	'Character.isLowerCase', 'Character.isMirrored', 'Character.isSpaceChar',
	'Character.isSupplementaryCodePoint', 'Character.isSurrogate', 'Character.isSurrogatePair',
	'Character.isTitleCase', 'Character.isUnicodeIdentifierPart',
	'Character.isUnicodeIdentifierStart', 'Character.isUpperCase', 'Character.isValidCodePoint',
	'Character.isWhitespace', 'Character.offsetByCodePoints', 'Character.reverseBytes',
	'Character.toChars', 'Character.toCodePoint', 'Character.toLowerCase', 'Character.toTitleCase',
	'Character.toUpperCase', 'Character.valueOf', --
	'Double.compare', 'Double.doubleToLongBits', 'Double.doubleToRawLongBits', 'Double.isInfinite',
	'Double.longBitsToDouble', 'Double.parseDouble', 'Double.toHexString', 'Double.valueOf', --
	'Integer.bitCount', 'Integer.compare', 'Integer.decode', 'Integer.getInteger',
	'Integer.highestOneBit', 'Integer.lowestOneBit', 'Integer.numberOfLeadingZeros',
	'Integer.numberOfTrailingZeros', 'Integer.parseInt', 'Integer.reverse', 'Integer.reverseBytes',
	'Integer.rotateLeft', 'Integer.rotateRight', 'Integer.signum', 'Integer.toBinaryString',
	'Integer.toHexString', 'Integer.toOctalString', 'Integer.valueOf', --
	'Math.abs', 'Math.acos', 'Math.asin', 'Math.atan', 'Math.atan2', 'Math.cbrt', 'Math.ceil',
	'Math.copySign', 'Math.cos', 'Math.cosh', 'Math.exp', 'Math.expm1', 'Math.floor',
	'Math.getExponent', 'Math.hypot', 'Math.IEEEremainder', 'Math.log', 'Math.log10', 'Math.log1p',
	'Math.max', 'Math.min', 'Math.nextAfter', 'Math.nextUp', 'Math.pow', 'Math.random', 'Math.rint',
	'Math.round', 'Math.scalb', 'Math.signum', 'Math.sin', 'Math.sinh', 'Math.sqrt', 'Math.tan',
	'Math.tanh', 'Math.toDegrees', 'Math.toRadians', 'Math.ulp', --
	'Runtime.getRuntime', --
	'String.copyValueOf', 'String.format', 'String.valueOf', --
	'System.arraycopy', 'System.clearProperty', 'System.console', 'System.currentTimeMillis',
	'System.exit', 'System.gc', 'System.getenv', 'System.getProperties', 'System.getProperty',
	'System.getSecurityManager', 'System.identityHashCode', 'System.inheritedChannel',
	'System.lineSeparator', 'System.load', 'System.loadLibrary', 'System.mapLibraryName',
	'System.nanoTime', 'System.runFinalization', 'System.setErr', 'System.setIn', 'System.setOut',
	'System.setProperties', 'System.setProperty', 'System.setSecurityManager', --
	'Thread.activeCount', 'Thread.currentThread', 'Thread.dumpStack', 'Thread.enumerate',
	'Thread.getAllStackTraces', 'Thread.getDefaultUncaughtExceptionHandler', 'Thread.holdsLock',
	'Thread.interrupted', 'Thread.setDefaultUncaughtExceptionHandler', 'Thread.sleep', 'Thread.yield' --
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'Double.MAX_EXPONENT', 'Double.MAX_VALUE', 'Double.MIN_EXPONENT', 'Double.MIN_NORMAL',
	'Double.MIN_VALUE', 'Double.NaN', 'Double.NEGATIVE_INFINITY', 'Double.POSITIVE_INFINITY', --
	'Integer.MAX_VALUE', 'Integer.MIN_VALUE', --
	'Math.E', 'Math.PI', --
	'Thread.MAX_PRIORITY', 'Thread.MIN_PRIORITY', 'Thread.NORM_PRIORITY'
})

lex:set_word_list(lexer.TYPE, {
	'boolean', 'byte', 'char', 'double', 'float', 'int', 'long', 'short', 'void', 'Boolean', 'Byte',
	'Character', 'Class', 'Double', 'Enum', 'Float', 'Integer', 'Long', 'Object', 'Process',
	'Runtime', 'Short', 'String', 'StringBuffer', 'StringBuilder', 'Thread', 'Throwable', 'Void',
	-- Exceptions.
	'ArithmeticException', 'ArrayIndexOutOfBoundsException', 'ArrayStoreException',
	'ClassCastException', 'ClassNotFoundException', 'CloneNotSupportedException',
	'EnumConstantNotPresentException', 'Exception', 'IllegalAccessException',
	'IllegalArgumentException', 'IllegalMonitorStateException', 'IllegalStateException',
	'IllegalThreadStateException', 'IndexOutOfBoundsException', 'InstantiationException',
	'InterruptedException', 'NegativeArraySizeException', 'NoSuchFieldException',
	'NoSuchMethodException', 'NullPointerException', 'NumberFormatException',
	'ReflectiveOperationException', 'RuntimeException', 'SecurityException',
	'StringIndexOutOfBoundsException', 'TypeNotPresentException', 'UnsupportedOperationException',
	-- Errors.
	'AbstractMethodError', 'AssertionError', 'BootstrapMethodError', 'ClassCircularityError',
	'ClassFormatError', 'Error', 'ExceptionInInitializerError', 'IllegalAccessError',
	'IncompatibleClassChangeError', 'InstantiationError', 'InternalError', 'LinkageError',
	'NoClassDefFoundError', 'NoSuchFieldError', 'NoSuchMethodError', 'OutOfMemoryError',
	'StackOverflowError', 'ThreadDeath', 'UnknownError', 'UnsatisfiedLinkError',
	'UnsupportedClassVersionError', 'VerifyError', 'VirtualMachineError'
})

lexer.property['scintillua.comment'] = '//'

return lex
