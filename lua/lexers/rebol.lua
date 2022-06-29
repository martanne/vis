-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Rebol LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rebol')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = 'comment' * P(' ')^-1 * lexer.range('{', '}')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'abs', 'absolute', 'add', 'and~', 'at', 'back', 'change', 'clear', 'complement', 'copy', 'cp',
  'divide', 'fifth', 'find', 'first', 'fourth', 'head', 'insert', 'last', 'make', 'max', 'maximum',
  'min', 'minimum', 'multiply', 'negate', 'next', 'or~', 'pick', 'poke', 'power', 'random',
  'remainder', 'remove', 'second', 'select', 'skip', 'sort', 'subtract', 'tail', 'third', 'to',
  'trim', 'xor~', --
  'alias', 'all', 'any', 'arccosine', 'arcsine', 'arctangent', 'bind', 'break', 'browse', 'call',
  'caret-to-offset', 'catch', 'checksum', 'close', 'comment', 'compose', 'compress', 'cosine',
  'debase', 'decompress', 'dehex', 'detab', 'dh-compute-key', 'dh-generate-key', 'dh-make-key',
  'difference', 'disarm', 'do', 'dsa-generate-key', 'dsa-make-key', 'dsa-make-signature',
  'dsa-verify-signature', 'either', 'else', 'enbase', 'entab', 'exclude', 'exit', 'exp', 'foreach',
  'form', 'free', 'get', 'get-modes', 'halt', 'hide', 'if', 'in', 'intersect', 'load', 'log-10',
  'log-2', 'log-e', 'loop', 'lowercase', 'maximum-of', 'minimum-of', 'mold', 'not', 'now',
  'offset-to-caret', 'open', 'parse', 'prin', 'print', 'protect', 'q', 'query', 'quit', 'read',
  'read-io', 'recycle', 'reduce', 'repeat', 'return', 'reverse', 'rsa-encrypt', 'rsa-generate-key',
  'rsa-make-key', 'save', 'secure', 'set', 'set-modes', 'show', 'sine', 'size-text', 'square-root',
  'tangent', 'textinfo', 'throw', 'to-hex', 'to-local-file', 'to-rebol-file', 'trace', 'try',
  'union', 'unique', 'unprotect', 'unset', 'until', 'update', 'uppercase', 'use', 'wait', 'while',
  'write', 'write-io', --
  'basic-syntax-header', 'crlf', 'font-fixed', 'font-sans-serif', 'font-serif', 'list-words',
  'outstr', 'val', 'value', --
  'about', 'alert', 'alter', 'append', 'array', 'ask', 'boot-prefs', 'build-tag', 'center-face',
  'change-dir', 'charset', 'choose', 'clean-path', 'clear-fields', 'confine', 'confirm', 'context',
  'cvs-date', 'cvs-version', 'decode-cgi', 'decode-url', 'deflag-face', 'delete', 'demo', 'desktop',
  'dirize', 'dispatch', 'do-boot', 'do-events', 'do-face', 'do-face-alt', 'does', 'dump-face',
  'dump-pane', 'echo', 'editor', 'emailer', 'emit', 'extract', 'find-by-type', 'find-key-face',
  'find-window', 'flag-face', 'flash', 'focus', 'for', 'forall', 'forever', 'forskip', 'func',
  'function', 'get-net-info', 'get-style', 'has', 'help', 'hide-popup', 'import-email', 'inform',
  'input', 'insert-event-func', 'join', 'launch', 'launch-thru', 'layout', 'license', 'list-dir',
  'load-image', 'load-prefs', 'load-thru', 'make-dir', 'make-face', 'net-error', 'open-events',
  'parse-email-addrs', 'parse-header', 'parse-header-date', 'parse-xml', 'path-thru', 'probe',
  'protect-system', 'read-net', 'read-thru', 'reboot', 'reform', 'rejoin', 'remold',
  'remove-event-func', 'rename', 'repend', 'replace', 'request', 'request-color', 'request-date',
  'request-download', 'request-file', 'request-list', 'request-pass', 'request-text', 'resend',
  'save-prefs', 'save-user', 'scroll-para', 'send', 'set-font', 'set-net', 'set-para', 'set-style',
  'set-user', 'set-user-name', 'show-popup', 'source', 'split-path', 'stylize', 'switch',
  'throw-on-error', 'to-binary', 'to-bitset', 'to-block', 'to-char', 'to-date', 'to-decimal',
  'to-email', 'to-event', 'to-file', 'to-get-word', 'to-hash', 'to-idate', 'to-image', 'to-integer',
  'to-issue', 'to-list', 'to-lit-path', 'to-lit-word', 'to-logic', 'to-money', 'to-none', 'to-pair',
  'to-paren', 'to-path', 'to-refinement', 'to-set-path', 'to-set-word', 'to-string', 'to-tag',
  'to-time', 'to-tuple', 'to-url', 'to-word', 'unfocus', 'uninstall', 'unview', 'upgrade', 'Usage',
  'vbug', 'view', 'view-install', 'view-prefs', 'what', 'what-dir', 'write-user', 'return', 'at',
  'space', 'pad', 'across', 'below', 'origin', 'guide', 'tabs', 'indent', 'style', 'styles', 'size',
  'sense', 'backcolor', 'do', 'none', --
  'action?', 'any-block?', 'any-function?', 'any-string?', 'any-type?', 'any-word?', 'binary?',
  'bitset?', 'block?', 'char?', 'datatype?', 'date?', 'decimal?', 'email?', 'empty?', 'equal?',
  'error?', 'even?', 'event?', 'file?', 'function?', 'get-word?', 'greater-or-equal?', 'greater?',
  'hash?', 'head?', 'image?', 'index?', 'integer?', 'issue?', 'length?', 'lesser-or-equal?',
  'lesser?', 'library?', 'list?', 'lit-path?', 'lit-word?', 'logic?', 'money?', 'native?',
  'negative?', 'none?', 'not-equal?', 'number?', 'object?', 'odd?', 'op?', 'pair?', 'paren?',
  'path?', 'port?', 'positive?', 'refinement?', 'routine?', 'same?', 'series?', 'set-path?',
  'set-word?', 'strict-equal?', 'strict-not-equal?', 'string?', 'struct?', 'tag?', 'tail?', 'time?',
  'tuple?', 'unset?', 'url?', 'word?', 'zero?', 'connected?', 'crypt-strength?', 'exists-key?',
  'input?', 'script?', 'type?', 'value?', '?', '??', 'dir?', 'exists-thru?', 'exists?',
  'flag-face?', 'found?', 'in-window?', 'info?', 'inside?', 'link-app?', 'link?', 'modified?',
  'offset?', 'outside?', 'screen-offset?', 'size?', 'span?', 'view?', 'viewed?', 'win-offset?',
  'within?', 'action!', 'any-block!', 'any-function!', 'any-string!', 'any-type!', 'any-word!',
  'binary!', 'bitset!', 'block!', 'char!', 'datatype!', 'date!', 'decimal!', 'email!', 'error!',
  'event!', 'file!', 'function!', 'get-word!', 'hash!', 'image!', 'integer!', 'issue!', 'library!',
  'list!', 'lit-path!', 'lit-word!', 'logic!', 'money!', 'native!', 'none!', 'number!', 'object!',
  'op!', 'pair!', 'paren!', 'path!', 'port!', 'refinement!', 'routine!', 'series!', 'set-path!',
  'set-word!', 'string!', 'struct!', 'symbol!', 'tag!', 'time!', 'tuple!', 'unset!', 'url!',
  'word!', --
  'true', 'false', 'self'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '-') * (lexer.alnum + '-')^0))

-- Strings.
local dq_str = lexer.range('"', true)
local br_str = lexer.range('{', '}', false, false, true)
local word_str = "'" * lexer.word
lex:add_rule('string', token(lexer.STRING, dq_str + br_str + word_str))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=<>+/*:()[]')))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines(';'))
lex:add_fold_point(lexer.OPERATOR, '[', ']')

return lex
