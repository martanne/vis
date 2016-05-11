-- Copyright 2006-2016 Mitchell mitchell.att.foicica.com. See LICENSE.
-- C LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'ansi_c'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = P('L')^-1 * l.delimited_range("'", true)
local dq_str = P('L')^-1 * l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Preprocessor.
local preproc_word = word_match{
  'define', 'elif', 'else', 'endif', 'if', 'ifdef', 'ifndef', 'include', 'line',
  'pragma', 'undef'
}
local preproc = token(l.PREPROCESSOR,
                      l.starts_line('#') * S('\t ')^0 * preproc_word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'auto', 'break', 'case', 'const', 'continue', 'default', 'do', 'else',
  'extern', 'for', 'goto', 'if', 'inline', 'register', 'restrict', 'return',
  'sizeof', 'static', 'switch', 'typedef', 'volatile', 'while'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  -- file descriptors
  'stdin', 'stdout', 'stderr',
  -- defines from stdlib.h
  'EXIT_SUCCESS', 'EXIT_FAILURE', 'RAND_MAX', 'NULL',
  -- error codes
  'EPERM', 'ENOENT', 'ESRCH', 'EINTR', 'EIO', 'ENXIO', 'E2BIG', 'ENOEXEC',
  'EBADF', 'ECHILD', 'EAGAIN', 'ENOMEM', 'EACCES', 'EFAULT', 'ENOTBLK',
  'EBUSY', 'EEXIST', 'EXDEV', 'ENODEV', 'ENOTDIR', 'EISDIR', 'EINVAL',
  'ENFILE', 'EMFILE', 'ENOTTY', 'ETXTBSY', 'EFBIG', 'ENOSPC', 'ESPIPE',
  'EROFS', 'EMLINK', 'EPIPE', 'EDOM', 'ERANGE',
  -- standard predefined macros
  '__DATE__', '__FILE__', '__LINE__', '__TIME__'
})

-- Types.
local type = token(l.TYPE, word_match{
  'char', 'double', 'enum', 'float', 'int', 'long', 'short', 'signed', 'struct',
  'union', 'unsigned', 'void', '_Bool', '_Complex', '_Imaginary'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>~!=^&|?~:;,.()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'constant', constant},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'preproc', preproc},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'%l+', '[{}]', '/%*', '%*/', '//'},
  [l.PREPROCESSOR] = {['if'] = 1, ifdef = 1, ifndef = 1, endif = -1},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
