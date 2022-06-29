-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Scheme LPeg lexer.
-- Contributions by Murray Calavera.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('scheme')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'and', 'or', 'not', 'else',
  --
  'library', 'define-library', 'export', 'include-library-declarations', 'cond-expand', 'import',
  'rename', 'only', 'except', 'prefix', 'include', 'include-ci',
  --
  'begin', 'case', 'case-lambda', 'cond', 'define', 'define-record-type', 'define-syntax',
  'define-values', 'delay', 'delay-force', 'do', 'if', 'guard', 'lambda', 'let', 'let*',
  'let*-values', 'let-syntax', 'let-values', 'letrec', 'letrec*', 'letrec-syntax', 'parameterize',
  'quasiquote', 'quote', 'set!', 'unless', 'unquote', 'unquote-splicing', 'when',
  --
  'define-macro', 'fluid-let'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  '*', '+', '-', '/', '<', '<=', '=', '=>', '>', '>=', 'abs', 'append', 'apply', 'assoc', 'assq',
  'assv', 'binary-port?', 'boolean=?', 'boolean?', 'bytevector', 'bytevector-append',
  'bytevector-copy', 'bytevector-copy!', 'bytevector-length', 'bytevector-u8-ref',
  'bytevector-u8-set!', 'bytevector?', 'caar', 'cadr', 'call-with-current-continuation',
  'call-with-port', 'call-with-values', 'call/cc', 'car', 'cdar', 'cddr', 'cdr', 'ceiling',
  'char->integer', 'char-ready?', 'char<=?', 'char<?', 'char=?', 'char>=?', 'char>?', 'char?',
  'close-input-port', 'close-output-port', 'close-port', 'complex?', 'cons', 'current-error-port',
  'current-input-port', 'current-output-port', 'denominator', 'dynamic-wind', 'eof-object',
  'eof-object?', 'eq?', 'equal?', 'eqv?', 'error', 'error-object-irritants', 'error-object-message',
  'error-object?', 'even?', 'exact', 'exact-integer-sqrt', 'exact-integer?', 'exact?', 'expt',
  'features', 'file-error?', 'floor', 'floor-quotient', 'floor-remainder', 'floor/',
  'flush-output-port', 'for-each', 'gcd', 'get-output-bytevector', 'get-output-string', 'inexact',
  'inexact?', 'input-port-open?', 'input-port?', 'integer->char', 'integer?', 'lcm', 'length',
  'list', 'list->string', 'list->vector', 'list-copy', 'list-ref', 'list-set!', 'list-tail',
  'list?', 'make-bytevector', 'make-list', 'make-parameter', 'make-string', 'make-vector', 'map',
  'max', 'member', 'memq', 'memv', 'min', 'modulo', 'negative?', 'newline', 'null?',
  'number->string', 'number?', 'numerator', 'odd?', 'open-input-bytevector', 'open-input-string',
  'open-output-bytevector', 'open-output-string', 'output-port-open?', 'output-port?', 'pair?',
  'peek-char', 'peek-u8', 'port?', 'positive?', 'procedure?', 'quotient', 'raise',
  'raise-continuable', 'rational?', 'rationalize', 'read-bytevector', 'read-bytevector!',
  'read-char', 'read-error?', 'read-line', 'read-string', 'read-u8', 'real?', 'remainder',
  'reverse', 'round', 'set-car!', 'set-cdr!', 'square', 'string', 'string->list', 'string->number',
  'string->symbol', 'string->utf8', 'string->vector', 'string-append', 'string-copy',
  'string-copy!', 'string-fill!', 'string-for-each', 'string-length', 'string-map', 'string-ref',
  'string-set!', 'string<=?', 'string<?', 'string=?', 'string>=?', 'string>?', 'string?',
  'substring', 'symbol->string', 'symbol=?', 'symbol?', 'syntax-error', 'syntax-rules',
  'textual-port?', 'truncate', 'truncate-quotient', 'truncate-remainder', 'truncate/', 'u8-ready?',
  'utf8->string', 'values', 'vector', 'vector->list', 'vector->string', 'vector-append',
  'vector-copy', 'vector-copy!', 'vector-fill!', 'vector-for-each', 'vector-length', 'vector-map',
  'vector-ref', 'vector-set!', 'vector?', 'with-exception-handler', 'write-bytevector',
  'write-char', 'write-string', 'write-u8', 'zero?',
  --
  'char-alphabetic?', 'char-ci<=?', 'char-ci<?', 'char-ci=?', 'char-ci>=?', 'char-ci>?',
  'char-downcase', 'char-foldcase', 'char-lower-case?', 'char-numeric?', 'char-upcase',
  'char-upper-case?', 'char-whitespace?', 'digit-value', 'string-ci<=?', 'string-ci<?',
  'string-ci=?', 'string-ci>=?', 'string-ci>?', 'string-downcase', 'string-foldcase',
  'string-upcase',
  --
  'angle', 'imag-part', 'magnitude', 'make-polar', 'make-rectangular', 'real-part',
  --
  'caaaar', 'caaadr', 'caaar', 'caadar', 'caaddr', 'caadr', 'cadaar', 'cadadr', 'cadar', 'caddar',
  'cadddr', 'caddr', 'cdaaar', 'cdaadr', 'cdaar', 'cdadar', 'cdaddr', 'cdadr', 'cddaar', 'cddadr',
  'cddar', 'cdddar', 'cddddr', 'cdddr',
  --
  'environment', 'eval',
  --
  'call-with-input-file', 'call-with-output-file', 'delete-file', 'file-exists?',
  'open-binary-input-file', 'open-binary-output-file', 'open-input-file', 'open-output-file',
  'with-input-from-file', 'with-output-to-file',
  --
  'acos', 'asin', 'atan', 'cos', 'exp', 'finite?', 'infinite?', 'log', 'nan?', 'sin', 'sqrt', 'tan',
  --
  'force', 'make-promise', 'promise?',
  --
  'load',
  --
  'command-line', 'emergency-exit', 'exit', 'get-environment-variable', 'get-environment-variables',
  --
  'read',
  --
  'interaction-environment',
  --
  'current-jiffy', 'current-second', 'jiffies-per-second',
  --
  'display', 'write', 'write-shared', 'write-simple',
  --
  'syntax-case', 'er-macro-transformer', 'sc-macro-transformer', 'rsc-macro-transformer'
}))

-- Identifiers and symbols.
local explicit_sign = S('+-')
local initial = lexer.alpha + S('!$%&*/:<=>?@^_~')
local subsequent = initial + lexer.digit + explicit_sign + '.'
local sign_subsequent = initial + explicit_sign
local dot_subsequent = sign_subsequent + '.'
-- LuaFormatter off
local peculiar_identifier =
  explicit_sign * '.' * dot_subsequent * subsequent^0 +
  explicit_sign * sign_subsequent * subsequent^0 +
  '.' * dot_subsequent * subsequent^0 +
  explicit_sign
-- LuaFormatter on
local ident = lexer.range('|') + initial * subsequent^0 + peculiar_identifier
lex:add_rule('identifier', token(lexer.IDENTIFIER, ident))
lex:add_rule('symbol', token(lexer.CLASS, "'" * ident))

-- Strings.
local character = '#\\' *
  (word_match('alarm backspace delete escape newline null return space tab') + 'x' * lexer.xdigit^1 +
    lexer.any)
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, character + dq_str))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match('#t #f #true #false')))

-- Directives.
lex:add_rule('directive', token(lexer.PREPROCESSOR, P('#!fold-case') + '#!no-fold-case'))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = lexer.range('#|', '|#', false, false, true)
local datum_comment = '#;' * lexer.space^0 * lexer.range('(', ')', false, true, true) *
  (lexer.any - lexer.space)^0
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment + datum_comment))

-- Numbers.
local radixes = {[2] = P('#b'), [8] = P('#o'), [10] = P('#d')^-1, [16] = P('#x')}
local digits = {[2] = S('01'), [8] = lpeg.R('07'), [10] = lexer.digit, [16] = lexer.xdigit}
local function num(r)
  local exactness = (P('#i') + '#e')^-1
  local radix, digit = radixes[r], digits[r]
  local prefix = radix * exactness + exactness * radix
  local suffix = ('e' * S('+-')^-1 * lexer.digit^1)^-1
  local infnan = S('+-') * word_match[[inf nan]] * '.0'
  -- LuaFormatter off
  local decimal = lexer.digit^1 * suffix +
    '.' * lexer.digit^1 * suffix +
    lexer.digit^1 * '.' * lexer.digit^0 * suffix
  local ureal = digit^1 * '/' * digit^1 +
    (r == 10 and decimal or P(false)) +
    digit^1
  local real = S('+-')^-1 * ureal + infnan
  local i = P('i')
  local complex = real * '@' * real +
    real * S('+-') * ureal^-1 * i +
    real * infnan * i +
    infnan * i +
    real +
    S('+-') * ureal^-1 * i
  -- LuaFormatter on
  return prefix * complex
end
lex:add_rule('number', token(lexer.NUMBER, num(2) + num(8) + num(10) + num(16)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, P('#u8') + ',@' + S(".`'#(),")))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.COMMENT, '#|', '|#')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines(';'))

return lex
