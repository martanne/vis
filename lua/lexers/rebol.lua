-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Rebol LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'rebol'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = ';' * l.nonnewline^0;
local block_comment = 'comment' * P(' ')^-1 *
                      l.delimited_range('{}', false, true)
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sl_string = l.delimited_range('"', true)
local ml_string = l.delimited_range('{}')
local lit_string = "'" * l.word
local string = token(l.STRING, sl_string + ml_string + lit_string)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'abs', 'absolute', 'add', 'and~', 'at', 'back', 'change', 'clear',
  'complement', 'copy', 'cp', 'divide', 'fifth', 'find', 'first', 'fourth',
  'head', 'insert', 'last', 'make', 'max', 'maximum', 'min', 'minimum',
  'multiply', 'negate', 'next', 'or~', 'pick', 'poke', 'power', 'random',
  'remainder', 'remove', 'second', 'select', 'skip', 'sort', 'subtract', 'tail',
  'third', 'to', 'trim', 'xor~', 'alias', 'all', 'any', 'arccosine', 'arcsine',
  'arctangent', 'bind', 'break', 'browse', 'call', 'caret-to-offset', 'catch',
  'checksum', 'close', 'comment', 'compose', 'compress', 'cosine', 'debase',
  'decompress', 'dehex', 'detab', 'dh-compute-key', 'dh-generate-key',
  'dh-make-key', 'difference', 'disarm', 'do', 'dsa-generate-key',
  'dsa-make-key', 'dsa-make-signature', 'dsa-verify-signature', 'either',
  'else', 'enbase', 'entab', 'exclude', 'exit', 'exp', 'foreach', 'form',
  'free', 'get', 'get-modes', 'halt', 'hide', 'if', 'in', 'intersect', 'load',
  'log-10', 'log-2', 'log-e', 'loop', 'lowercase', 'maximum-of', 'minimum-of',
  'mold', 'not', 'now', 'offset-to-caret', 'open', 'parse', 'prin', 'print',
  'protect', 'q', 'query', 'quit', 'read', 'read-io', 'recycle', 'reduce',
  'repeat', 'return', 'reverse', 'rsa-encrypt', 'rsa-generate-key',
  'rsa-make-key', 'save', 'secure', 'set', 'set-modes', 'show', 'sine',
  'size-text', 'square-root', 'tangent', 'textinfo', 'throw', 'to-hex',
  'to-local-file', 'to-rebol-file', 'trace', 'try', 'union', 'unique',
  'unprotect', 'unset', 'until', 'update', 'uppercase', 'use', 'wait', 'while',
  'write', 'write-io', 'basic-syntax-header', 'crlf', 'font-fixed',
  'font-sans-serif', 'font-serif', 'list-words', 'outstr', 'val', 'value',
  'about', 'alert', 'alter', 'append', 'array', 'ask', 'boot-prefs',
  'build-tag', 'center-face', 'change-dir', 'charset', 'choose', 'clean-path',
  'clear-fields', 'confine', 'confirm', 'context', 'cvs-date', 'cvs-version',
  'decode-cgi', 'decode-url', 'deflag-face', 'delete', 'demo', 'desktop',
  'dirize', 'dispatch', 'do-boot', 'do-events', 'do-face', 'do-face-alt',
  'does', 'dump-face', 'dump-pane', 'echo', 'editor', 'emailer', 'emit',
  'extract', 'find-by-type', 'find-key-face', 'find-window', 'flag-face',
  'flash', 'focus', 'for', 'forall', 'forever', 'forskip', 'func', 'function',
  'get-net-info', 'get-style', 'has', 'help', 'hide-popup', 'import-email',
  'inform', 'input', 'insert-event-func', 'join', 'launch', 'launch-thru',
  'layout', 'license', 'list-dir', 'load-image', 'load-prefs', 'load-thru',
  'make-dir', 'make-face', 'net-error', 'open-events', 'parse-email-addrs',
  'parse-header', 'parse-header-date', 'parse-xml', 'path-thru', 'probe',
  'protect-system', 'read-net', 'read-thru', 'reboot', 'reform', 'rejoin',
  'remold', 'remove-event-func', 'rename', 'repend', 'replace', 'request',
  'request-color', 'request-date', 'request-download', 'request-file',
  'request-list', 'request-pass', 'request-text', 'resend', 'save-prefs',
  'save-user', 'scroll-para', 'send', 'set-font', 'set-net', 'set-para',
  'set-style', 'set-user', 'set-user-name', 'show-popup', 'source',
  'split-path', 'stylize', 'switch', 'throw-on-error', 'to-binary',
  'to-bitset', 'to-block', 'to-char', 'to-date', 'to-decimal', 'to-email',
  'to-event', 'to-file', 'to-get-word', 'to-hash', 'to-idate', 'to-image',
  'to-integer', 'to-issue', 'to-list', 'to-lit-path', 'to-lit-word', 'to-logic',
  'to-money', 'to-none', 'to-pair', 'to-paren', 'to-path', 'to-refinement',
  'to-set-path', 'to-set-word', 'to-string', 'to-tag', 'to-time', 'to-tuple',
  'to-url', 'to-word', 'unfocus', 'uninstall', 'unview', 'upgrade', 'Usage',
  'vbug', 'view', 'view-install', 'view-prefs', 'what', 'what-dir',
  'write-user', 'return', 'at', 'space', 'pad', 'across', 'below', 'origin',
  'guide', 'tabs', 'indent', 'style', 'styles', 'size', 'sense', 'backcolor',
  'do', 'none',
  'action?', 'any-block?', 'any-function?', 'any-string?', 'any-type?',
  'any-word?', 'binary?', 'bitset?', 'block?', 'char?', 'datatype?', 'date?',
  'decimal?', 'email?', 'empty?', 'equal?', 'error?', 'even?', 'event?',
  'file?', 'function?', 'get-word?', 'greater-or-equal?', 'greater?', 'hash?',
  'head?', 'image?', 'index?', 'integer?', 'issue?', 'length?',
  'lesser-or-equal?', 'lesser?', 'library?', 'list?', 'lit-path?', 'lit-word?',
  'logic?', 'money?', 'native?', 'negative?', 'none?', 'not-equal?', 'number?',
  'object?', 'odd?', 'op?', 'pair?', 'paren?', 'path?', 'port?', 'positive?',
  'refinement?', 'routine?', 'same?', 'series?', 'set-path?', 'set-word?',
  'strict-equal?', 'strict-not-equal?', 'string?', 'struct?', 'tag?', 'tail?',
  'time?', 'tuple?', 'unset?', 'url?', 'word?', 'zero?', 'connected?',
  'crypt-strength?', 'exists-key?', 'input?', 'script?', 'type?', 'value?', '?',
  '??', 'dir?', 'exists-thru?', 'exists?', 'flag-face?', 'found?', 'in-window?',
  'info?', 'inside?', 'link-app?', 'link?', 'modified?', 'offset?', 'outside?',
  'screen-offset?', 'size?', 'span?', 'view?', 'viewed?', 'win-offset?',
  'within?',
  'action!', 'any-block!', 'any-function!', 'any-string!', 'any-type!',
  'any-word!', 'binary!', 'bitset!', 'block!', 'char!', 'datatype!', 'date!',
  'decimal!', 'email!', 'error!', 'event!', 'file!', 'function!', 'get-word!',
  'hash!', 'image!', 'integer!', 'issue!', 'library!', 'list!', 'lit-path!',
  'lit-word!', 'logic!', 'money!', 'native!', 'none!', 'number!', 'object!',
  'op!', 'pair!', 'paren!', 'path!', 'port!', 'refinement!', 'routine!',
  'series!', 'set-path!', 'set-word!', 'string!', 'struct!', 'symbol!', 'tag!',
  'time!', 'tuple!', 'unset!', 'url!', 'word!',
  'true', 'false', 'self'
}, '~-?!'))

-- Identifiers.
local word = (l.alpha + '-') * (l.alnum + '-')^0
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('=<>+/*:()[]'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[%[%]{}]', ';'},
  [l.COMMENT] = {['{'] = 1, ['}'] = -1, [';'] = l.fold_line_comments(';')},
  [l.OPERATOR] = {['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1}
}

return M
