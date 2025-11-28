-- Copyright 2025 Jason Lenz code@engenforge.com. See LICENSE.
-- Janet LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Much of the lexical syntax defined below was derived from the following two
-- Janet documentation links:
-- https://janet-lang.org/docs/syntax.html
-- https://janet-lang.org/api/index.html

-- Note: In some cases Janet documentation uses terminology that differs with
-- Scintillua typical usage. For example, the Janet documentation defines
-- keywords as symbols that begin with the character ':' and are treated by the
-- compiler as constants. In this case they were tagged in Scintillua as
-- constants. Conversely, keywords in Scintillua were defined as built in names
-- reserved by the janet compiler such as nil, true, do, fn, etc.

-- Keywords.
local shorthand = S("',;~|")
local lead_ch = S('([{') + shorthand + lexer.space
local trail_ch = S(')]}') + lexer.space
lex:add_rule('keyword',
	lex:tag(lexer.KEYWORD, B(lead_ch) * lex:word_match(lexer.KEYWORD) * #trail_ch))

-- Functions.
lex:add_rule('function', lex:tag(lexer.FUNCTION_BUILTIN,
	B(lead_ch) * lex:word_match(lexer.FUNCTION_BUILTIN) * #trail_ch))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER,
	B(lead_ch) * S('-+')^-1 * lexer.digit^1 * (S('._&') + lexer.alnum)^0))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S("()[]{}") + shorthand + P('@') * S('([{')))

-- Constants.
local id_ch = S('!@$%^&*:-_+=<>.?') + lexer.alnum -- + lpeg.utfR(0x7F, 0x10FFFF)
lex:add_rule('constant', lex:tag(lexer.CONSTANT, ':' * id_ch^0))

-- Strings and buffers.
local dq_str = lexer.range('"', false, true)
local bt_str = lpeg.Cmt(P('`')^1, function(input, index, bt)
	local _, e = input:find(bt, index)
	return (e or #input) + 1
end)
lex:add_rule('string', lex:tag(lexer.STRING, P('@')^-1 * (dq_str + bt_str)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, id_ch^1))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'break', 'def', 'do', 'false', 'fn', 'if', 'nil', 'quasiquote', 'quote', 'set', 'slice', 'true',
	'unquote', 'var', 'while'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'%', '%=', '*', '*=', '*args*', '*current-file*', '*debug*', '*defdyn-prefix*', '*doc-color*',
	'*doc-width*', '*err*', '*err-color*', '*executable*', '*exit*', '*exit-value*', '*ffi-context*',
	'*lint-error*', '*lint-levels*', '*lint-warn*', '*macro-form*', '*macro-lints*', '*module-cache*',
	'*module-loaders*', '*module-loading*', '*module-make-env*', '*module-paths*', '*out*',
	'*peg-grammar*', '*pretty-format*', '*profilepath*', '*redef*', '*repl-prompt*', '*syspath*',
	'*task-id*', '+', '++', '+=', '-', '--', '-=', '->', '->>', '-?>', '-?>>', '/', '/=', '<', '<=',
	'=', '>', '>=', 'abstract?', 'accumulate', 'accumulate2', 'all', 'all-bindings', 'all-dynamics',
	'and', 'any?', 'apply', 'array', 'array/clear', 'array/concat', 'array/ensure', 'array/fill',
	'array/insert', 'array/join', 'array/new', 'array/new-filled', 'array/peek', 'array/pop',
	'array/push', 'array/remove', 'array/slice', 'array/trim', 'array/weak', 'array?', 'as->',
	'as-macro', 'as?->', 'asm', 'assert', 'assertf', 'bad-compile', 'bad-parse', 'band', 'blshift',
	'bnot', 'boolean?', 'bor', 'brshift', 'brushift', 'buffer', 'buffer/bit', 'buffer/bit-clear',
	'buffer/bit-set', 'buffer/bit-toggle', 'buffer/blit', 'buffer/clear', 'buffer/fill',
	'buffer/format', 'buffer/format-at', 'buffer/from-bytes', 'buffer/new', 'buffer/new-filled',
	'buffer/popn', 'buffer/push', 'buffer/push-at', 'buffer/push-byte', 'buffer/push-float32',
	'buffer/push-float64', 'buffer/push-string', 'buffer/push-uint16', 'buffer/push-uint32',
	'buffer/push-uint64', 'buffer/push-word', 'buffer/slice', 'buffer/trim', 'buffer?', 'bundle/add',
	'bundle/add-bin', 'bundle/add-directory', 'bundle/add-file', 'bundle/install',
	'bundle/installed?', 'bundle/list', 'bundle/manifest', 'bundle/prune', 'bundle/reinstall',
	'bundle/replace', 'bundle/topolist', 'bundle/uninstall', 'bundle/update-all', 'bundle/whois',
	'bxor', 'bytes?', 'cancel', 'case', 'catseq', 'cfunction?', 'chr', 'cli-main', 'cmp', 'comment',
	'comp', 'compare', 'compare<', 'compare<=', 'compare=', 'compare>', 'compare>=', 'compif',
	'compile', 'complement', 'comptime', 'compwhen', 'cond', 'coro', 'count', 'curenv', 'debug',
	'debug/arg-stack', 'debug/break', 'debug/fbreak', 'debug/lineage', 'debug/stack',
	'debug/stacktrace', 'debug/step', 'debug/unbreak', 'debug/unfbreak', 'debugger', 'debugger-env',
	'debugger-on-status', 'dec', 'deep-not=', 'deep=', 'def-', 'default', 'default-peg-grammar',
	'defdyn', 'defer', 'defglobal', 'defmacro', 'defmacro-', 'defn', 'defn-', 'delay', 'describe',
	'dictionary?', 'disasm', 'distinct', 'div', 'doc', 'doc*', 'doc-format', 'doc-of', 'dofile',
	'drop', 'drop-until', 'drop-while', 'dyn', 'each', 'eachk', 'eachp', 'edefer', 'eflush', 'empty?',
	'env-lookup', 'eprin', 'eprinf', 'eprint', 'eprintf', 'error', 'errorf', 'ev/acquire-lock',
	'ev/acquire-rlock', 'ev/acquire-wlock', 'ev/all-tasks', 'ev/call', 'ev/cancel', 'ev/capacity',
	'ev/chan', 'ev/chan-close', 'ev/chunk', 'ev/close', 'ev/count', 'ev/deadline', 'ev/do-thread',
	'ev/full', 'ev/gather', 'ev/give', 'ev/give-supervisor', 'ev/go', 'ev/lock', 'ev/read',
	'ev/release-lock', 'ev/release-rlock', 'ev/release-wlock', 'ev/rselect', 'ev/rwlock', 'ev/select',
	'ev/sleep', 'ev/spawn', 'ev/spawn-thread', 'ev/take', 'ev/thread', 'ev/thread-chan', 'ev/to-file',
	'ev/with-deadline', 'ev/with-lock', 'ev/with-rlock', 'ev/with-wlock', 'ev/write', 'eval',
	'eval-string', 'even?', 'every?', 'extreme', 'false?', 'ffi/align', 'ffi/call',
	'ffi/calling-conventions', 'ffi/close', 'ffi/context', 'ffi/defbind', 'ffi/defbind-alias',
	'ffi/free', 'ffi/jitfn', 'ffi/lookup', 'ffi/malloc', 'ffi/native', 'ffi/pointer-buffer',
	'ffi/pointer-cfunction', 'ffi/read', 'ffi/signature', 'ffi/size', 'ffi/struct', 'ffi/trampoline',
	'ffi/write', 'fiber-fn', 'fiber/can-resume?', 'fiber/current', 'fiber/getenv', 'fiber/last-value',
	'fiber/maxstack', 'fiber/new', 'fiber/root', 'fiber/setenv', 'fiber/setmaxstack', 'fiber/status',
	'fiber?', 'file/close', 'file/flush', 'file/lines', 'file/open', 'file/read', 'file/seek',
	'file/tell', 'file/temp', 'file/write', 'filewatch/add', 'filewatch/listen', 'filewatch/new',
	'filewatch/remove', 'filewatch/unlisten', 'filter', 'find', 'find-index', 'first', 'flatten',
	'flatten-into', 'flush', 'flycheck', 'for', 'forever', 'forv', 'freeze', 'frequencies',
	'from-pairs', 'function?', 'gccollect', 'gcinterval', 'gcsetinterval', 'generate', 'gensym',
	'geomean', 'get', 'get-in', 'getline', 'getproto', 'group-by', 'has-key?', 'has-value?', 'hash',
	'idempotent?', 'identity', 'if-let', 'if-not', 'if-with', 'import', 'import*', 'in', 'inc',
	'index-of', 'indexed?', 'int/s64', 'int/to-bytes', 'int/to-number', 'int/u64', 'int?',
	'interleave', 'interpose', 'invert', 'janet/build', 'janet/config-bits', 'janet/version', 'juxt',
	'juxt*', 'keep', 'keep-syntax', 'keep-syntax!', 'keys', 'keyword', 'keyword/slice', 'keyword?',
	'kvs', 'label', 'last', 'length', 'lengthable?', 'let', 'load-image', 'load-image-dict', 'loop',
	'macex', 'macex1', 'maclintf', 'make-env', 'make-image', 'make-image-dict', 'map', 'mapcat',
	'marshal', 'match', 'math/-inf', 'math/abs', 'math/acos', 'math/acosh', 'math/asin', 'math/asinh',
	'math/atan', 'math/atan2', 'math/atanh', 'math/cbrt', 'math/ceil', 'math/cos', 'math/cosh',
	'math/e', 'math/erf', 'math/erfc', 'math/exp', 'math/exp2', 'math/expm1', 'math/floor',
	'math/frexp', 'math/gamma', 'math/gcd', 'math/hypot', 'math/inf', 'math/int-max', 'math/int-min',
	'math/int32-max', 'math/int32-min', 'math/lcm', 'math/ldexp', 'math/log', 'math/log-gamma',
	'math/log10', 'math/log1p', 'math/log2', 'math/nan', 'math/next', 'math/pi', 'math/pow',
	'math/random', 'math/rng', 'math/rng-buffer', 'math/rng-int', 'math/rng-uniform', 'math/round',
	'math/seedrandom', 'math/sin', 'math/sinh', 'math/sqrt', 'math/tan', 'math/tanh', 'math/trunc',
	'max', 'max-of', 'mean', 'memcmp', 'merge', 'merge-into', 'merge-module', 'min', 'min-of', 'mod',
	'module/add-paths', 'module/cache', 'module/expand-path', 'module/find', 'module/loaders',
	'module/loading', 'module/paths', 'module/value', 'nan?', 'nat?', 'native', 'neg?', 'net/accept',
	'net/accept-loop', 'net/address', 'net/address-unpack', 'net/chunk', 'net/close', 'net/connect',
	'net/flush', 'net/listen', 'net/localname', 'net/peername', 'net/read', 'net/recv-from',
	'net/send-to', 'net/server', 'net/setsockopt', 'net/shutdown', 'net/write', 'next', 'nil?', 'not',
	'not=', 'number?', 'odd?', 'one?', 'or', 'os/arch', 'os/cd', 'os/chmod', 'os/clock',
	'os/compiler', 'os/cpu-count', 'os/cryptorand', 'os/cwd', 'os/date', 'os/dir', 'os/environ',
	'os/execute', 'os/exit', 'os/getenv', 'os/isatty', 'os/link', 'os/lstat', 'os/mkdir', 'os/mktime',
	'os/open', 'os/perm-int', 'os/perm-string', 'os/pipe', 'os/posix-exec', 'os/posix-fork',
	'os/proc-close', 'os/proc-kill', 'os/proc-wait', 'os/readlink', 'os/realpath', 'os/rename',
	'os/rm', 'os/rmdir', 'os/setenv', 'os/setlocale', 'os/shell', 'os/sigaction', 'os/sleep',
	'os/spawn', 'os/stat', 'os/strftime', 'os/symlink', 'os/time', 'os/touch', 'os/umask', 'os/which',
	'pairs', 'parse', 'parse-all', 'parser/byte', 'parser/clone', 'parser/consume', 'parser/eof',
	'parser/error', 'parser/flush', 'parser/has-more', 'parser/insert', 'parser/new',
	'parser/produce', 'parser/state', 'parser/status', 'parser/where', 'partial', 'partition',
	'partition-by', 'peg/compile', 'peg/find', 'peg/find-all', 'peg/match', 'peg/replace',
	'peg/replace-all', 'pos?', 'postwalk', 'pp', 'prewalk', 'prin', 'prinf', 'print', 'printf',
	'product', 'prompt', 'propagate', 'protect', 'put', 'put-in', 'quit', 'range', 'reduce',
	'reduce2', 'repeat', 'repl', 'require', 'resume', 'return', 'reverse', 'reverse!', 'root-env',
	'run-context', 'sandbox', 'scan-number', 'seq', 'setdyn', 'short-fn', 'signal', 'slice', 'slurp',
	'some', 'sort', 'sort-by', 'sorted', 'sorted-by', 'spit', 'stderr', 'stdin', 'stdout', 'string',
	'string/ascii-lower', 'string/ascii-upper', 'string/bytes', 'string/check-set', 'string/find',
	'string/find-all', 'string/format', 'string/from-bytes', 'string/has-prefix?',
	'string/has-suffix?', 'string/join', 'string/repeat', 'string/replace', 'string/replace-all',
	'string/reverse', 'string/slice', 'string/split', 'string/trim', 'string/triml', 'string/trimr',
	'string?', 'struct', 'struct/getproto', 'struct/proto-flatten', 'struct/rawget',
	'struct/to-table', 'struct/with-proto', 'struct?', 'sum', 'symbol', 'symbol/slice', 'symbol?',
	'table', 'table/clear', 'table/clone', 'table/getproto', 'table/new', 'table/proto-flatten',
	'table/rawget', 'table/setproto', 'table/to-struct', 'table/weak', 'table/weak-keys',
	'table/weak-values', 'table?', 'tabseq', 'take', 'take-until', 'take-while', 'thaw', 'toggle',
	'trace', 'tracev', 'true?', 'truthy?', 'try', 'tuple', 'tuple/brackets', 'tuple/join',
	'tuple/setmap', 'tuple/slice', 'tuple/sourcemap', 'tuple/type', 'tuple?', 'type', 'unless',
	'unmarshal', 'untrace', 'update', 'update-in', 'use', 'values', 'var-', 'varfn', 'varglobal',
	'walk', 'warn-compile', 'when', 'when-let', 'when-with', 'with', 'with-dyns', 'with-env',
	'with-syms', 'with-vars', 'xprin', 'xprinf', 'xprint', 'xprintf', 'yield', 'zero?', 'zipcoll'
})

lexer.property['scintillua.comment'] = '#'

return lex
