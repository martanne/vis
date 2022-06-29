-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Altered for Janet by sevvie Rose
-- Janet LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'janet'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '#' * l.nonnewline^0
local comment = token(l.COMMENT, line_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"'))

-- Numbers.
local number = token(l.NUMBER, P('-')^-1 * l.digit^1 * (S('./') * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'def', 'var', 'fn', 'do', 'quote', 'if', 'splice', 'while', 'break', 'set',
  'quasiquote', 'unquote', 'upscope', 
}, '-*!'))

-- Functions.
local func = token(l.FUNCTION, word_match({
  '%', '%=', '*', '*=', '*args*', '*current-file*', '*debug*', '*defdyn-prefix*', 
  '*doc-color*', '*doc-width*', '*err*', '*err-color*', '*executable*', '*exit*', 
  '*exit-value*', '*ffi-context*', '*lint-error*', '*lint-levels*', 
  '*lint-warn*', '*macro-form*', '*macro-lints*', '*out*', '*peg-grammar*', 
  '*pretty-format*', '*profilepath*', '*redef*', '*syspath*', '+', '++', '+=', 
  '-', '--', '-=', '->', '->>', '-?>', '-?>>', '/', '/=', '<', '<=', '=', '>', 
  '>=', 'abstract?', 'accumulate', 'accumulate2', 'all', 'all-bindings', 
  'all-dynamics', 'and', 'any?', 'apply', 'array', 'array/clear', 'array/concat', 
  'array/ensure', 'array/fill', 'array/insert', 'array/new', 'array/new-filled', 
  'array/peek', 'array/pop', 'array/push', 'array/remove', 'array/slice', 
  'array/trim', 'array?', 'as->', 'as-macro', 'as?->', 'asm', 'assert', 
  'bad-compile', 'bad-parse', 'band', 'blshift', 'bnot', 'boolean?', 'bor', 
  'brshift', 'brushift', 'buffer', 'buffer/bit', 'buffer/bit-clear', 
  'buffer/bit-set', 'buffer/bit-toggle', 'buffer/blit', 'buffer/clear', 
  'buffer/fill', 'buffer/format', 'buffer/new', 'buffer/new-filled', 
  'buffer/popn', 'buffer/push', 'buffer/push-byte', 'buffer/push-string', 
  'buffer/push-word', 'buffer/slice', 'buffer/trim', 'buffer?', 'bxor', 'bytes?', 
  'cancel', 'case', 'cfunction?', 'chr', 'cli-main', 'cmp', 'comment', 'comp', 
  'compare', 'compare<', 'compare<=', 'compare=', 'compare>', 'compare>=', 
  'compif', 'compile', 'complement', 'comptime', 'compwhen', 'cond', 'coro', 
  'count', 'curenv', 'debug', 'debug/arg-stack', 'debug/break', 'debug/fbreak', 
  'debug/lineage', 'debug/stack', 'debug/stacktrace', 'debug/step', 
  'debug/unbreak', 'debug/unfbreak', 'debugger', 'debugger-env', 
  'debugger-on-status', 'dec', 'deep-not=', 'deep=', 'def-', 'default', 
  'default-peg-grammar', 'defdyn', 'defer', 'defglobal', 'defmacro', 'defmacro-', 
  'defn', 'defn-', 'delay', 'describe', 'dictionary?', 'disasm', 'distinct', 
  'doc', 'doc*', 'doc-format', 'doc-of', 'dofile', 'drop', 'drop-until', 
  'drop-while', 'dyn', 'each', 'eachk', 'eachp', 'edefer', 'eflush', 'empty?', 
  'env-lookup', 'eprin', 'eprinf', 'eprint', 'eprintf', 'error', 'errorf', 
  'ev/acquire-lock', 'ev/acquire-rlock', 'ev/acquire-wlock', 'ev/call', 
  'ev/cancel', 'ev/capacity', 'ev/chan', 'ev/chan-close', 'ev/chunk', 'ev/close', 
  'ev/count', 'ev/deadline', 'ev/do-thread', 'ev/full', 'ev/gather', 'ev/give', 
  'ev/give-supervisor', 'ev/go', 'ev/lock', 'ev/read', 'ev/release-lock', 
  'ev/release-rlock', 'ev/release-wlock', 'ev/rselect', 'ev/rwlock', 'ev/select', 
  'ev/sleep', 'ev/spawn', 'ev/spawn-thread', 'ev/take', 'ev/thread', 
  'ev/thread-chan', 'ev/with-deadline', 'ev/write', 'eval', 'eval-string', 
  'even?', 'every?', 'extreme', 'false?', 'ffi/align', 'ffi/call', 'ffi/close', 
  'ffi/context', 'ffi/defbind', 'ffi/lookup', 'ffi/native', 'ffi/read', 
  'ffi/signature', 'ffi/size', 'ffi/struct', 'ffi/trampoline', 'ffi/write', 
  'fiber-fn', 'fiber/can-resume?', 'fiber/current', 'fiber/getenv', 
  'fiber/last-value', 'fiber/maxstack', 'fiber/new', 'fiber/root', 
  'fiber/setenv', 'fiber/setmaxstack', 'fiber/status', 'fiber?', 'file/close', 
  'file/flush', 'file/open', 'file/read', 'file/seek', 'file/temp', 'file/write', 
  'filter', 'find', 'find-index', 'first', 'flatten', 'flatten-into', 'flush', 
  'flycheck', 'for', 'forever', 'forv', 'freeze', 'frequencies', 'from-pairs', 
  'function?', 'gccollect', 'gcinterval', 'gcsetinterval', 'generate', 'gensym', 
  'get', 'get-in', 'getline', 'getproto', 'group-by', 'hash', 'idempotent?', 
  'identity', 'if-let', 'if-not', 'if-with', 'import', 'import*', 'in', 'inc', 
  'index-of', 'indexed?', 'int/s64', 'int/to-bytes', 'int/to-number', 'int/u64', 
  'int?', 'interleave', 'interpose', 'invert', 'janet/build', 
  'janet/config-bits', 'janet/version', 'juxt', 'juxt*', 'keep', 'keys', 
  'keyword', 'keyword/slice', 'keyword?', 'kvs', 'label', 'last', 'length', 
  'let', 'load-image', 'load-image-dict', 'loop', 'macex', 'macex1', 'maclintf', 
  'make-env', 'make-image', 'make-image-dict', 'map', 'mapcat', 'marshal', 
  'match', 'math/-inf', 'math/abs', 'math/acos', 'math/acosh', 'math/asin', 
  'math/asinh', 'math/atan', 'math/atan2', 'math/atanh', 'math/cbrt', 
  'math/ceil', 'math/cos', 'math/cosh', 'math/e', 'math/erf', 'math/erfc', 
  'math/exp', 'math/exp2', 'math/expm1', 'math/floor', 'math/gamma', 'math/gcd', 
  'math/hypot', 'math/inf', 'math/int-max', 'math/int-min', 'math/int32-max', 
  'math/int32-min', 'math/lcm', 'math/log', 'math/log-gamma', 'math/log10', 
  'math/log1p', 'math/log2', 'math/nan', 'math/next', 'math/pi', 'math/pow', 
  'math/random', 'math/rng', 'math/rng-buffer', 'math/rng-int', 
  'math/rng-uniform', 'math/round', 'math/seedrandom', 'math/sin', 'math/sinh', 
  'math/sqrt', 'math/tan', 'math/tanh', 'math/trunc', 'max', 'max-of', 'mean', 
  'merge', 'merge-into', 'merge-module', 'min', 'min-of', 'mod', 
  'module/add-paths', 'module/cache', 'module/expand-path', 'module/find', 
  'module/loaders', 'module/loading', 'module/paths', 'module/value', 'nan?', 
  'nat?', 'native', 'neg?', 'net/accept', 'net/accept-loop', 'net/address', 
  'net/address-unpack', 'net/chunk', 'net/close', 'net/connect', 'net/flush', 
  'net/listen', 'net/localname', 'net/peername', 'net/read', 'net/recv-from', 
  'net/send-to', 'net/server', 'net/shutdown', 'net/write', 'next', 'nil?', 
  'not', 'not=', 'number?', 'odd?', 'one?', 'or', 'os/arch', 'os/cd', 'os/chmod', 
  'os/clock', 'os/cpu-count', 'os/cryptorand', 'os/cwd', 'os/date', 'os/dir', 
  'os/environ', 'os/execute', 'os/exit', 'os/getenv', 'os/link', 'os/lstat', 
  'os/mkdir', 'os/mktime', 'os/open', 'os/perm-int', 'os/perm-string', 'os/pipe', 
  'os/proc-close', 'os/proc-kill', 'os/proc-wait', 'os/readlink', 'os/realpath', 
  'os/rename', 'os/rm', 'os/rmdir', 'os/setenv', 'os/shell', 'os/sleep', 
  'os/spawn', 'os/stat', 'os/symlink', 'os/time', 'os/touch', 'os/umask', 
  'os/which', 'pairs', 'parse', 'parse-all', 'parser/byte', 'parser/clone', 
  'parser/consume', 'parser/eof', 'parser/error', 'parser/flush', 
  'parser/has-more', 'parser/insert', 'parser/new', 'parser/produce', 
  'parser/state', 'parser/status', 'parser/where', 'partial', 'partition', 
  'partition-by', 'peg/compile', 'peg/find', 'peg/find-all', 'peg/match', 
  'peg/replace', 'peg/replace-all', 'pos?', 'postwalk', 'pp', 'prewalk', 'prin', 
  'prinf', 'print', 'printf', 'product', 'prompt', 'propagate', 'protect', 'put', 
  'put-in', 'quit', 'range', 'reduce', 'reduce2', 'repeat', 'repl', 'require', 
  'resume', 'return', 'reverse', 'reverse!', 'root-env', 'run-context', 
  'scan-number', 'seq', 'setdyn', 'short-fn', 'signal', 'slice', 'slurp', 'some', 
  'sort', 'sort-by', 'sorted', 'sorted-by', 'spit', 'stderr', 'stdin', 'stdout', 
  'string', 'string/ascii-lower', 'string/ascii-upper', 'string/bytes', 
  'string/check-set', 'string/find', 'string/find-all', 'string/format', 
  'string/from-bytes', 'string/has-prefix?', 'string/has-suffix?', 'string/join', 
  'string/repeat', 'string/replace', 'string/replace-all', 'string/reverse', 
  'string/slice', 'string/split', 'string/trim', 'string/triml', 'string/trimr', 
  'string?', 'struct', 'struct/getproto', 'struct/proto-flatten', 
  'struct/to-table', 'struct/with-proto', 'struct?', 'sum', 'symbol', 
  'symbol/slice', 'symbol?', 'table', 'table/clear', 'table/clone', 
  'table/getproto', 'table/new', 'table/proto-flatten', 'table/rawget', 
  'table/setproto', 'table/to-struct', 'table?', 'take', 'take-until', 
  'take-while', 'trace', 'tracev', 'true?', 'truthy?', 'try', 'tuple', 
  'tuple/brackets', 'tuple/setmap', 'tuple/slice', 'tuple/sourcemap', 
  'tuple/type', 'tuple?', 'type', 'unless', 'unmarshal', 'untrace', 'update', 
  'update-in', 'use', 'values', 'var-', 'varfn', 'varglobal', 'walk', 
  'warn-compile', 'when', 'when-let', 'when-with', 'with', 'with-dyns', 
  'with-syms', 'with-vars', 'xprin', 'xprinf', 'xprint', 'xprintf', 'yield', 
  'zero?', 'zipcoll'
}, '-/<>!?=#\''))

-- Identifiers.
local word = (l.alpha + S('-!?*$=-')) * (l.alnum + S('.-!?*$+-'))^0
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, S('`@()'))

-- Clojure keywords
local janet_keyword = token('janet_keyword', ':' * S(':')^-1 * word * ('/' * word )^-1)
local janet_symbol = token('janet_symbol', "\'" * word * ('/' * word )^-1 )

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'func', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
  {"janet_keyword", janet_keyword},
  {"janet_symbol", janet_symbol}
}


M._tokenstyles = {
  janet_keyword = l.STYLE_TYPE,
  janet_symbol = l.STYLE_TYPE..',bold',
}

M._foldsymbols = {
  _patterns = {'[%(%)%[%]{}]',  ';'},
  [l.OPERATOR] = {
    ['('] = 1, [')'] = -1, ['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1
  },
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
