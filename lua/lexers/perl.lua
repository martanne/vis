-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Perl LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = {_NAME = 'perl'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '#' * l.nonnewline_esc^0
local block_comment = l.starts_line('=') * l.alpha *
                      (l.any - l.newline * '=cut')^0 * (l.newline * '=cut')^-1
local comment = token(l.COMMENT, block_comment + line_comment)

local delimiter_matches = {['('] = ')', ['['] = ']', ['{'] = '}', ['<'] = '>'}
local literal_delimitted = P(function(input, index) -- for single delimiter sets
  local delimiter = input:sub(index, index)
  if not delimiter:find('%w') then -- only non alpha-numerics
    local match_pos, patt
    if delimiter_matches[delimiter] then
      -- Handle nested delimiter/matches in strings.
      local s, e = delimiter, delimiter_matches[delimiter]
      patt = l.delimited_range(s..e, false, false, true)
    else
      patt = l.delimited_range(delimiter)
    end
    match_pos = lpeg.match(patt, input, index)
    return match_pos or #input + 1
  end
end)
local literal_delimitted2 = P(function(input, index) -- for 2 delimiter sets
  local delimiter = input:sub(index, index)
  -- Only consider non-alpha-numerics and non-spaces as delimiters. The
  -- non-spaces are used to ignore operators like "-s".
  if not delimiter:find('[%w ]') then
    local match_pos, patt
    if delimiter_matches[delimiter] then
      -- Handle nested delimiter/matches in strings.
      local s, e = delimiter, delimiter_matches[delimiter]
      patt = l.delimited_range(s..e, false, false, true)
    else
      patt = l.delimited_range(delimiter)
    end
    first_match_pos = lpeg.match(patt, input, index)
    if not first_match_pos then
      return #input + 1
    end
    final_match_pos = lpeg.match(patt, input, first_match_pos - 1)
    if not final_match_pos then -- using (), [], {}, or <> notation
      final_match_pos = lpeg.match(l.space^0 * patt, input, first_match_pos)
    end
    return final_match_pos or #input + 1
  end
end)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local cmd_str = l.delimited_range('`')
local heredoc = '<<' * P(function(input, index)
  local s, e, delimiter = input:find('([%a_][%w_]*)[\n\r\f;]+', index)
  if s == index and delimiter then
    local end_heredoc = '[\n\r\f]+'
    local _, e = input:find(end_heredoc..delimiter, e)
    return e and e + 1 or #input + 1
  end
end)
local lit_str = 'q' * P('q')^-1 * literal_delimitted
local lit_array = 'qw' * literal_delimitted
local lit_cmd = 'qx' * literal_delimitted
local lit_tr = (P('tr') + 'y') * literal_delimitted2 * S('cds')^0
local regex_str = #P('/') * l.last_char_includes('-<>+*!~\\=%&|^?:;([{') *
                  l.delimited_range('/', true) * S('imosx')^0
local lit_regex = 'qr' * literal_delimitted * S('imosx')^0
local lit_match = 'm' * literal_delimitted * S('cgimosx')^0
local lit_sub = 's' * literal_delimitted2 * S('ecgimosx')^0
local string = token(l.STRING, sq_str + dq_str + cmd_str + heredoc + lit_str +
                               lit_array + lit_cmd + lit_tr) +
               token(l.REGEX, regex_str + lit_regex + lit_match + lit_sub)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'STDIN', 'STDOUT', 'STDERR', 'BEGIN', 'END', 'CHECK', 'INIT',
  'require', 'use',
  'break', 'continue', 'do', 'each', 'else', 'elsif', 'foreach', 'for', 'if',
  'last', 'local', 'my', 'next', 'our', 'package', 'return', 'sub', 'unless',
  'until', 'while', '__FILE__', '__LINE__', '__PACKAGE__',
  'and', 'or', 'not', 'eq', 'ne', 'lt', 'gt', 'le', 'ge'
})

-- Functions.
local func = token(l.FUNCTION, word_match({
  'abs', 'accept', 'alarm', 'atan2', 'bind', 'binmode', 'bless', 'caller',
  'chdir', 'chmod', 'chomp', 'chop', 'chown', 'chr', 'chroot', 'closedir',
  'close', 'connect', 'cos', 'crypt', 'dbmclose', 'dbmopen', 'defined',
  'delete', 'die', 'dump', 'each', 'endgrent', 'endhostent', 'endnetent',
  'endprotoent', 'endpwent', 'endservent', 'eof', 'eval', 'exec', 'exists',
  'exit', 'exp', 'fcntl', 'fileno', 'flock', 'fork', 'format', 'formline',
  'getc', 'getgrent', 'getgrgid', 'getgrnam', 'gethostbyaddr', 'gethostbyname',
  'gethostent', 'getlogin', 'getnetbyaddr', 'getnetbyname', 'getnetent',
  'getpeername', 'getpgrp', 'getppid', 'getpriority', 'getprotobyname',
  'getprotobynumber', 'getprotoent', 'getpwent', 'getpwnam', 'getpwuid',
  'getservbyname', 'getservbyport', 'getservent', 'getsockname', 'getsockopt',
  'glob', 'gmtime', 'goto', 'grep', 'hex', 'import', 'index', 'int', 'ioctl',
  'join', 'keys', 'kill', 'lcfirst', 'lc', 'length', 'link', 'listen',
  'localtime', 'log', 'lstat', 'map', 'mkdir', 'msgctl', 'msgget', 'msgrcv',
  'msgsnd', 'new', 'oct', 'opendir', 'open', 'ord', 'pack', 'pipe', 'pop',
  'pos', 'printf', 'print', 'prototype', 'push', 'quotemeta', 'rand', 'readdir',
  'read', 'readlink', 'recv', 'redo', 'ref', 'rename', 'reset', 'reverse',
  'rewinddir', 'rindex', 'rmdir', 'scalar', 'seekdir', 'seek', 'select',
  'semctl', 'semget', 'semop', 'send', 'setgrent', 'sethostent', 'setnetent',
  'setpgrp', 'setpriority', 'setprotoent', 'setpwent', 'setservent',
  'setsockopt', 'shift', 'shmctl', 'shmget', 'shmread', 'shmwrite', 'shutdown',
  'sin', 'sleep', 'socket', 'socketpair', 'sort', 'splice', 'split', 'sprintf',
  'sqrt', 'srand', 'stat', 'study', 'substr', 'symlink', 'syscall', 'sysread',
  'sysseek', 'system', 'syswrite', 'telldir', 'tell', 'tied', 'tie', 'time',
  'times', 'truncate', 'ucfirst', 'uc', 'umask', 'undef', 'unlink', 'unpack',
  'unshift', 'untie', 'utime', 'values', 'vec', 'wait', 'waitpid', 'wantarray',
  'warn', 'write'
}, '2'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local special_var = '$' * ('^' * S('ADEFHILMOPSTWX')^-1 +
                           S('\\"[]\'&`+*.,;=%~?@<>(|/!-') +
                           ':' * (l.any - ':') + P('$') * -l.word + l.digit^1)
local plain_var = ('$#' + S('$@%')) * P('$')^0 * l.word + '$#'
local variable = token(l.VARIABLE, special_var + plain_var)

-- Operators.
local operator = token(l.OPERATOR, S('-<>+*!~\\=/%&|^.?:;()[]{}'))

-- Markers.
local marker = token(l.COMMENT, word_match{'__DATA__', '__END__'} * l.any^0)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'marker', marker},
  {'function', func},
  {'string', string},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'variable', variable},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[%[%]{}]', '#'},
  [l.OPERATOR] = {['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
