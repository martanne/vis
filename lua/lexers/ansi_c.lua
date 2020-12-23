-- Copyright 2006-2020 Mitchell. See LICENSE.
-- C LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ansi_c')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  auto break case const continue default do else extern for goto if inline
  register restrict return sizeof static switch typedef volatile while
  -- C99.
  false true
  -- C11.
  _Alignas _Alignof _Atomic _Generic _Noreturn _Static_assert _Thread_local
]]))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match[[
  bool char double enum float int long short signed struct union unsigned void
  _Bool _Complex _Imaginary
  -- Stdlib types.
  ptrdiff_t size_t max_align_t wchar_t intptr_t uintptr_t intmax_t uintmax_t
]] + P('u')^-1 * 'int' * (P('_least') + '_fast')^-1 * lexer.digit^1 * '_t'))

-- Constants.
lex:add_rule('constants', token(lexer.CONSTANT, word_match[[
  NULL
  -- Preprocessor.
  __DATE__ __FILE__ __LINE__ __TIME__ __func__
  -- errno.h.
  E2BIG EACCES EADDRINUSE EADDRNOTAVAIL EAFNOSUPPORT EAGAIN EALREADY EBADF
  EBADMSG EBUSY ECANCELED ECHILD ECONNABORTED ECONNREFUSED ECONNRESET EDEADLK
  EDESTADDRREQ EDOM EDQUOT EEXIST EFAULT EFBIG EHOSTUNREACH EIDRM EILSEQ
  EINPROGRESS EINTR EINVAL EIO EISCONN EISDIR ELOOP EMFILE EMLINK EMSGSIZE
  EMULTIHOP ENAMETOOLONG ENETDOWN ENETRESET ENETUNREACH ENFILE ENOBUFS ENODATA
  ENODEV ENOENT ENOEXEC ENOLCK ENOLINK ENOMEM ENOMSG ENOPROTOOPT ENOSPC ENOSR
  ENOSTR ENOSYS ENOTCONN ENOTDIR ENOTEMPTY ENOTRECOVERABLE ENOTSOCK ENOTSUP
  ENOTTY ENXIO EOPNOTSUPP EOVERFLOW EOWNERDEAD EPERM EPIPE EPROTO
  EPROTONOSUPPORT EPROTOTYPE ERANGE EROFS ESPIPE ESRCH ESTALE ETIME ETIMEDOUT
  ETXTBSY EWOULDBLOCK EXDEV
]]))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = P('L')^-1 * lexer.range("'", true)
local dq_str = P('L')^-1 * lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/') +
  lexer.range('#if' * S(' \t')^0 * '0' * lexer.space, '#endif')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Preprocessor.
local include = token(lexer.PREPROCESSOR, '#' * S('\t ')^0 * 'include') *
  (ws * token(lexer.STRING, lexer.range('<', '>', true)))^-1
local preproc = token(lexer.PREPROCESSOR, '#' * S('\t ')^0 * word_match[[
  define elif else endif if ifdef ifndef line pragma undef
]])
lex:add_rule('preprocessor', include + preproc)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, '#if', '#endif')
lex:add_fold_point(lexer.PREPROCESSOR, '#ifdef', '#endif')
lex:add_fold_point(lexer.PREPROCESSOR, '#ifndef', '#endif')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
