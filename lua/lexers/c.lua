-- Copyright 2006-2025 Mitchell. See LICENSE.
-- C LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Functions.
local builtin_func = -(B('.') + B('->')) *
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = (B('.') + B('->')) * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Constants.
lex:add_rule('constants', lex:tag(lexer.CONSTANT_BUILTIN,
	-(B('.') + B('->')) * lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Labels.
lex:add_rule('label', lex:tag(lexer.LABEL, lexer.starts_line(lexer.word * ':')))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, P('L')^-1 * (sq_str + dq_str)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local ws = S(' \t')^0
local block_comment = lexer.range('/*', '*/') +
	lexer.range('#if' * ws * '0' * lexer.space, '#endif')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
local integer = lexer.integer * lexer.word_match('u l ll ul ull lu llu', true)^-1
local float = lexer.float * lexer.word_match('f l df dd dl i j', true)^-1
lex:add_rule('number', lex:tag(lexer.NUMBER, float + integer))

-- Preprocessor.
local include = lex:tag(lexer.PREPROCESSOR, '#' * ws * 'include') *
	(lex:get_rule('whitespace') * lex:tag(lexer.STRING, lexer.range('<', '>', true)))^-1
local preproc = lex:tag(lexer.PREPROCESSOR, '#' * ws * lex:word_match(lexer.PREPROCESSOR))
lex:add_rule('preprocessor', include + preproc)

-- Attributes.
local standard_attr = lex:word_match(lexer.ATTRIBUTE)
local non_standard_attr = lexer.word * '::' * lexer.word
local attr_args = lexer.range('(', ')')
local attr = (non_standard_attr + standard_attr) * attr_args^-1
lex:add_rule('attribute', lex:tag(lexer.ATTRIBUTE, '[[' * attr * (ws * ',' * ws * attr)^0 * ']]'))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, '#if', '#endif')
lex:add_fold_point(lexer.PREPROCESSOR, '#ifdef', '#endif')
lex:add_fold_point(lexer.PREPROCESSOR, '#ifndef', '#endif')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'auto', 'break', 'case', 'const', 'continue', 'default', 'do', 'else', 'enum', 'extern', 'for',
	'goto', 'if', 'inline', 'register', 'restrict', 'return', 'sizeof', 'static', 'switch', 'typedef',
	'volatile', 'while', --
	'false', 'true', -- C99
	'alignas', 'alignof', '_Atomic', '_Generic', 'noreturn', '_Static_assert', 'thread_local', -- C11
	-- Compiler.
	'asm', '__asm', '__asm__', '__restrict__', '__inline', '__inline__', '__attribute__', '__declspec'
})

lex:set_word_list(lexer.TYPE, {
	'bool', 'char', 'double', 'float', 'int', 'long', 'short', 'signed', 'struct', 'union',
	'unsigned', 'void', --
	'complex', 'imaginary', '_Complex', '_Imaginary', -- complex.h C99
	'lconv', -- locale.h
	'div_t', -- math.h
	'va_list', -- stdarg.h
	'bool', '_Bool', -- stdbool.h C99
	-- stddef.h.
	'size_t', 'ptrdiff_t', --
	'max_align_t', -- C11
	-- stdint.h.
	'int8_t', 'int16_t', 'int32_t', 'int64_t', 'int_fast8_t', 'int_fast16_t', 'int_fast32_t',
	'int_fast64_t', 'int_least8_t', 'int_least16_t', 'int_least32_t', 'int_least64_t', 'intmax_t',
	'intptr_t', 'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'uint_fast8_t', 'uint_fast16_t',
	'uint_fast32_t', 'uint_fast64_t', 'uint_least8_t', 'uint_least16_t', 'uint_least32_t',
	'uint_least64_t', 'uintmax_t', 'uintptr_t', --
	'FILE', 'fpos_t', -- stdio.h
	'div_t', 'ldiv_t', -- stdlib.h
	-- time.h.
	'tm', 'time_t', 'clock_t', --
	'timespec' -- C11
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'assert', -- assert.h
	-- complex.h.
	'CMPLX', 'creal', 'cimag', 'cabs', 'carg', 'conj', 'cproj',
	-- C99
	'cexp', 'cpow', 'csin', 'ccos', 'ctan', 'casin', 'cacos', 'catan', 'csinh', 'ccosh', 'ctanh',
	'casinh', 'cacosh', 'catanh',
	-- ctype.h.
	'isalnum', 'isalpha', 'islower', 'isupper', 'isdigit', 'isxdigit', 'iscntrl', 'isgraph',
	'isspace', 'isprint', 'ispunct', 'tolower', 'toupper', --
	'isblank', -- C99
	-- inttypes.h.
	'INT8_C', 'INT16_C', 'INT32_C', 'INT64_C', 'INTMAX_C', 'UINT8_C', 'UINT16_C', 'UINT32_C',
	'UINT64_C', 'UINTMAX_C', --
	'setlocale', 'localeconv', -- locale.h
	-- math.h.
	'abs', 'div', 'fabs', 'fmod', 'exp', 'log', 'log10', 'pow', 'sqrt', 'sin', 'cos', 'tan', 'asin',
	'acos', 'atan', 'atan2', 'sinh', 'cosh', 'tanh', 'ceil', 'floor', 'frexp', 'ldexp', 'modf',
	-- C99.
	'remainder', 'remquo', 'fma', 'fmax', 'fmin', 'fdim', 'nan', 'exp2', 'expm1', 'log2', 'log1p',
	'cbrt', 'hypot', 'asinh', 'acosh', 'atanh', 'erf', 'erfc', 'tgamma', 'lgamma', 'trunc', 'round',
	'nearbyint', 'rint', 'scalbn', 'ilogb', 'logb', 'nextafter', 'nexttoward', 'copysign', 'isfinite',
	'isinf', 'isnan', 'isnormal', 'signbit', 'isgreater', 'isgreaterequal', 'isless', 'islessequal',
	'islessgreater', 'isunordered', --
	'strtoimax', 'strtoumax', -- inttypes.h C99
	'signal', 'raise', -- signal.h
	'setjmp', 'longjmp', -- setjmp.h
	'va_start', 'va_arg', 'va_end', -- stdarg.h
	-- stdio.h.
	'fopen', 'freopen', 'fclose', 'fflush', 'setbuf', 'setvbuf', 'fwide', 'fread', 'fwrite', 'fgetc',
	'getc', 'fgets', 'fputc', 'putc', 'getchar', 'gets', 'putchar', 'puts', 'ungetc', 'scanf',
	'fscanf', 'sscanf', 'printf', 'fprintf', 'sprintf', 'vprintf', 'vfprintf', 'vsprintf', 'ftell',
	'fgetpos', 'fseek', 'fsetpos', 'rewind', 'clearerr', 'feof', 'ferror', 'perror', 'remove',
	'rename', 'tmpfile', 'tmpnam',
	-- stdlib.h.
	'abort', 'exit', 'atexit', 'system', 'getenv', 'malloc', 'calloc', 'realloc', 'free', 'atof',
	'atoi', 'atol', 'strtol', 'strtoul', 'strtod', 'mblen', 'mbsinit', 'mbrlen', 'qsort', 'bsearch',
	'rand', 'srand', --
	'quick_exit', '_Exit', 'at_quick_exit', 'aligned_alloc', -- C11
	-- string.h.
	'strcpy', 'strncpy', 'strcat', 'strncat', 'strxfrm', 'strlen', 'strcmp', 'strncmp', 'strcoll',
	'strchr', 'strrchr', 'strspn', 'strcspn', 'strpbrk', 'strstr', 'strtok', 'memchr', 'memcmp',
	'memset', 'memcpy', 'memmove', 'strerror',
	-- time.h.
	'difftime', 'time', 'clock', 'asctime', 'ctime', 'gmtime', 'localtime', 'mktime', --
	'timespec_get' -- C11
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'NULL', --
	'__DATE__', '__FILE__', '__LINE__', '__TIME__', '__func__', -- preprocessor
	-- errno.h.
	'errno', --
	'E2BIG', 'EACCES', 'EADDRINUSE', 'EADDRNOTAVAIL', 'EAFNOSUPPORT', 'EAGAIN', 'EALREADY', 'EBADF',
	'EBADMSG', 'EBUSY', 'ECANCELED', 'ECHILD', 'ECONNABORTED', 'ECONNREFUSED', 'ECONNRESET',
	'EDEADLK', 'EDESTADDRREQ', 'EDOM', 'EDQUOT', 'EEXIST', 'EFAULT', 'EFBIG', 'EHOSTUNREACH', 'EIDRM',
	'EILSEQ', 'EINPROGRESS', 'EINTR', 'EINVAL', 'EIO', 'EISCONN', 'EISDIR', 'ELOOP', 'EMFILE',
	'EMLINK', 'EMSGSIZE', 'EMULTIHOP', 'ENAMETOOLONG', 'ENETDOWN', 'ENETRESET', 'ENETUNREACH',
	'ENFILE', 'ENOBUFS', 'ENODATA', 'ENODEV', 'ENOENT', 'ENOEXEC', 'ENOLCK', 'ENOLINK', 'ENOMEM',
	'ENOMSG', 'ENOPROTOOPT', 'ENOSPC', 'ENOSR', 'ENOSTR', 'ENOSYS', 'ENOTCONN', 'ENOTDIR',
	'ENOTEMPTY', 'ENOTRECOVERABLE', 'ENOTSOCK', 'ENOTSUP', 'ENOTTY', 'ENXIO', 'EOPNOTSUPP',
	'EOVERFLOW', 'EOWNERDEAD', 'EPERM', 'EPIPE', 'EPROTO', 'EPROTONOSUPPORT', 'EPROTOTYPE', 'ERANGE',
	'EROFS', 'ESPIPE', 'ESRCH', 'ESTALE', 'ETIME', 'ETIMEDOUT', 'ETXTBSY', 'EWOULDBLOCK', 'EXDEV',
	-- float.h.
	'FLT_MIN', 'DBL_MIN', 'LDBL_MIN', 'FLT_MAX', 'DBL_MAX', 'LDBL_MAX',
	-- limits.h.
	'CHAR_BIT', 'MB_LEN_MAX', 'CHAR_MIN', 'CHAR_MAX', 'SCHAR_MIN', 'SHRT_MIN', 'INT_MIN', 'LONG_MIN',
	'SCHAR_MAX', 'SHRT_MAX', 'INT_MAX', 'LONG_MAX', 'UCHAR_MAX', 'USHRT_MAX', 'UINT_MAX', 'ULONG_MAX',
	-- C99.
	'LLONG_MIN', 'ULLONG_MAX', 'PTRDIFF_MIN', 'PTRDIFF_MAX', 'SIZE_MAX', 'SIG_ATOMIC_MIN',
	'SIG_ATOMIC_MAX', 'WINT_MIN', 'WINT_MAX', 'WCHAR_MIN', 'WCHAR_MAX', --
	'LC_ALL', 'LC_COLLATE', 'LC_CTYPE', 'LC_MONETARY', 'LC_NUMERIC', 'LC_TIME', -- locale.h
	-- math.h.
	'HUGE_VAL', --
	'INFINITY', 'NAN', -- C99
	-- stdint.h.
	'INT8_MIN', 'INT16_MIN', 'INT32_MIN', 'INT64_MIN', 'INT_FAST8_MIN', 'INT_FAST16_MIN',
	'INT_FAST32_MIN', 'INT_FAST64_MIN', 'INT_LEAST8_MIN', 'INT_LEAST16_MIN', 'INT_LEAST32_MIN',
	'INT_LEAST64_MIN', 'INTPTR_MIN', 'INTMAX_MIN', 'INT8_MAX', 'INT16_MAX', 'INT32_MAX', 'INT64_MAX',
	'INT_FAST8_MAX', 'INT_FAST16_MAX', 'INT_FAST32_MAX', 'INT_FAST64_MAX', 'INT_LEAST8_MAX',
	'INT_LEAST16_MAX', 'INT_LEAST32_MAX', 'INT_LEAST64_MAX', 'INTPTR_MAX', 'INTMAX_MAX', 'UINT8_MAX',
	'UINT16_MAX', 'UINT32_MAX', 'UINT64_MAX', 'UINT_FAST8_MAX', 'UINT_FAST16_MAX', 'UINT_FAST32_MAX',
	'UINT_FAST64_MAX', 'UINT_LEAST8_MAX', 'UINT_LEAST16_MAX', 'UINT_LEAST32_MAX', 'UINT_LEAST64_MAX',
	'UINTPTR_MAX', 'UINTMAX_MAX',
	-- stdio.h
	'stdin', 'stdout', 'stderr', 'EOF', 'FOPEN_MAX', 'FILENAME_MAX', 'BUFSIZ', '_IOFBF', '_IOLBF',
	'_IONBF', 'SEEK_SET', 'SEEK_CUR', 'SEEK_END', 'TMP_MAX', --
	'EXIT_SUCCESS', 'EXIT_FAILURE', 'RAND_MAX', -- stdlib.h
	-- signal.h.
	'SIG_DFL', 'SIG_IGN', 'SIG_ERR', 'SIGABRT', 'SIGFPE', 'SIGILL', 'SIGINT', 'SIGSEGV', 'SIGTERM', --
	'CLOCKS_PER_SEC' -- time.h.
})

lex:set_word_list(lexer.PREPROCESSOR, {
	'define', 'defined', 'elif', 'else', 'endif', 'error', 'if', 'ifdef', 'ifndef', 'line', 'pragma',
	'undef'
})

lex:set_word_list(lexer.ATTRIBUTE, {
	-- C23
	'deprecated', 'fallthrough', 'nodiscard', 'maybe_unused', 'noreturn', '_Noreturn', 'unsequenced',
	'reproducible'
})

lexer.property['scintillua.comment'] = '//'

return lex
