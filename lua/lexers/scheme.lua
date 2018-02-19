-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Scheme LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'scheme'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = ';' * l.nonnewline^0
local block_comment = l.nested_pair('#|', '|#')
-- TODO: this should handle any datum and take into account "#\)", ";" etc.
local datum_comment
  = P'#;' * l.space^0
  * (l.delimited_range("()", false, true, true) + (l.any - l.space)^1)
local comment = token(l.COMMENT, datum_comment + line_comment + block_comment)

-- Strings.
local character
  = P'#\\' * ( P'alarm' + P'backspace' + P'delete' + P'escape'
             + P'newline' + P'null' + P'return' + P'space' + P'tab')
  + P'#\\x' * l.xdigit^1
  + P'#\\' * P(1)
local dq_str = l.delimited_range('"')
local string = token(l.STRING, character + dq_str)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  "and", "or", "not", "else",

  "library", "define-library", "export", "include-library-declarations",
  "cond-expand", "import", "rename", "only", "except", "prefix", "include",
  "include-ci",

  "begin", "case", "case-lambda", "cond", "define", "define-record-type",
  "define-syntax", "define-values", "delay", "delay-force", "do", "if",
  "guard", "lambda", "let", "let*", "let*-values", "let-syntax", "let-values",
  "letrec", "letrec*", "letrec-syntax", "parameterize", "quasiquote", "quote",
  "set!", "unless", "unquote", "unquote-splicing", "when",

  "define-macro", "fluid-let"
}, '.-+!$%&*/:<=>?@^_~'))

-- Functions.
local func = token(l.FUNCTION, word_match({
  "*", "+", "-", "/", "<", "<=", "=", "=>", ">", ">=", "abs", "append",
  "apply", "assoc", "assq", "assv", "binary-port?", "boolean=?", "boolean?",
  "bytevector", "bytevector-append", "bytevector-copy", "bytevector-copy!",
  "bytevector-length", "bytevector-u8-ref", "bytevector-u8-set!",
  "bytevector?", "caar", "cadr", "call-with-current-continuation",
  "call-with-port", "call-with-values", "call/cc", "car", "cdar", "cddr",
  "cdr", "ceiling", "char->integer", "char-ready?", "char<=?", "char<?",
  "char=?", "char>=?", "char>?", "char?", "close-input-port",
  "close-output-port", "close-port", "complex?", "cons", "current-error-port",
  "current-input-port", "current-output-port", "denominator", "dynamic-wind",
  "eof-object", "eof-object?", "eq?", "equal?", "eqv?", "error",
  "error-object-irritants", "error-object-message", "error-object?", "even?",
  "exact", "exact-integer-sqrt", "exact-integer?", "exact?", "expt",
  "features", "file-error?", "floor", "floor-quotient", "floor-remainder",
  "floor/", "flush-output-port", "for-each", "gcd", "get-output-bytevector",
  "get-output-string", "inexact", "inexact?", "input-port-open?",
  "input-port?", "integer->char", "integer?", "lcm", "length", "list",
  "list->string", "list->vector", "list-copy", "list-ref", "list-set!",
  "list-tail", "list?", "make-bytevector", "make-list", "make-parameter",
  "make-string", "make-vector", "map", "max", "member", "memq", "memv", "min",
  "modulo", "negative?", "newline", "null?", "number->string", "number?",
  "numerator", "odd?", "open-input-bytevector", "open-input-string",
  "open-output-bytevector", "open-output-string", "output-port-open?",
  "output-port?", "pair?", "peek-char", "peek-u8", "port?", "positive?",
  "procedure?", "quotient", "raise", "raise-continuable", "rational?",
  "rationalize", "read-bytevector", "read-bytevector!", "read-char",
  "read-error?", "read-line", "read-string", "read-u8", "real?", "remainder",
  "reverse", "round", "set-car!", "set-cdr!", "square", "string",
  "string->list", "string->number", "string->symbol", "string->utf8",
  "string->vector", "string-append", "string-copy", "string-copy!",
  "string-fill!", "string-for-each", "string-length", "string-map",
  "string-ref", "string-set!", "string<=?", "string<?", "string=?",
  "string>=?", "string>?", "string?", "substring", "symbol->string",
  "symbol=?", "symbol?", "syntax-error", "syntax-rules", "textual-port?",
  "truncate", "truncate-quotient", "truncate-remainder", "truncate/",
  "u8-ready?", "utf8->string", "values", "vector", "vector->list",
  "vector->string", "vector-append", "vector-copy", "vector-copy!",
  "vector-fill!", "vector-for-each", "vector-length", "vector-map",
  "vector-ref", "vector-set!", "vector?", "with-exception-handler",
  "write-bytevector", "write-char", "write-string", "write-u8", "zero?",

  "char-alphabetic?", "char-ci<=?", "char-ci<?", "char-ci=?", "char-ci>=?",
  "char-ci>?", "char-downcase", "char-foldcase", "char-lower-case?",
  "char-numeric?", "char-upcase", "char-upper-case?", "char-whitespace?",
  "digit-value", "string-ci<=?", "string-ci<?", "string-ci=?", "string-ci>=?",
  "string-ci>?", "string-downcase", "string-foldcase", "string-upcase",

  "angle", "imag-part", "magnitude", "make-polar", "make-rectangular",
  "real-part",

  "caaaar", "caaadr", "caaar", "caadar", "caaddr", "caadr", "cadaar", "cadadr",
  "cadar", "caddar", "cadddr", "caddr", "cdaaar", "cdaadr", "cdaar", "cdadar",
  "cdaddr", "cdadr", "cddaar", "cddadr", "cddar", "cdddar", "cddddr", "cdddr",

  "environment", "eval",

  "call-with-input-file", "call-with-output-file", "delete-file",
  "file-exists?", "open-binary-input-file", "open-binary-output-file",
  "open-input-file", "open-output-file", "with-input-from-file",
  "with-output-to-file",

  "acos", "asin", "atan", "cos", "exp", "finite?", "infinite?", "log", "nan?",
  "sin", "sqrt", "tan",

  "force", "make-promise", "promise?",

  "load",

  "command-line", "emergency-exit", "exit", "get-environment-variable",
  "get-environment-variables",

  "read",

  "interaction-environment",

  "current-jiffy", "current-second", "jiffies-per-second",

  "display", "write", "write-shared", "write-simple",

  "syntax-case", "er-macro-transformer", "sc-macro-transformer",
  "rsc-macro-transformer"
}, '.-+!$%&*/:<=>?@^_~'))

local directive = token(l.PREPROCESSOR, P'#!fold-case' + P'#!no-fold-case')
local boolean = token(l.CONSTANT,
  word_match({'#t', '#f', '#true', '#false'}, '#'))

-- Identifiers.
local explicit_sign = S('+-')

local initial = l.alpha + S('!$%&*/:<=>?@^_~')
local subsequent = initial + l.digit + explicit_sign + P'.'

local sign_subsequent = initial + explicit_sign
local dot_subsequent = sign_subsequent + P'.'

local peculiar_identifier
  = explicit_sign * P'.' * dot_subsequent * subsequent^0
  + explicit_sign * sign_subsequent * subsequent^0
  + P'.' * dot_subsequent * subsequent^0
  + explicit_sign

local ident
  = l.delimited_range('|')
  + initial * subsequent^0
  + peculiar_identifier

local identifier = token(l.IDENTIFIER, ident)
local symbol = token(l.CLASS, P"'" * ident)

-- Numbers.
local function num(r)
  local exactness = (P'#i' + P'#e')^-1

  local radix = ({
    [2] = P'#b',
    [8] = P'#o',
    [10] = P('#d')^-1,
    [16] = P'#x'
  })[r]

  local digit = ({
    [2] = S'01',
    [8] = R'07',
    [10] = l.digit,
    [16] = l.xdigit
  })[r]

  local prefix = radix * exactness + exactness * radix
  local suffix = (P'e' * S('+-')^-1 * l.digit^1)^-1

  local infnan = P'+inf.0' + P'-inf.0' + P'+nan.0' + P'-nan.0'

  local decimal
    = l.digit^1 * suffix
    + P'.' * l.digit^1 * suffix
    + l.digit^1 * P'.' * l.digit^0 * suffix

  local ureal
    = digit^1 * P'/' * digit^1
    + (r == 10 and decimal or P(false))
    + digit^1
  local real
    = S('+-')^-1 * ureal
    + infnan

  local i = P'i'
  local complex
    = real * P'@' * real
    + real * S'+-' * ureal^-1 * i
    + real * infnan * i
    + infnan * i
    + real
    + S'+-' * ureal^-1 * i

  return prefix * complex
end

local number = token(l.NUMBER, num(2) + num(8) + num(10) + num(16))

-- Operators.
local operator = token(l.OPERATOR, P'#u8' + P',@' + S(".`'#(),"))

M._rules = {
  {'whitespace', ws},
  {'directive', directive},
  {'boolean', boolean},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'keyword', keyword},
  {'func', func},
  {'identifier', identifier},
  {'symbol', symbol},
  {'operator', operator},
}


M._foldsymbols = {
  _patterns = {'[%(%)%[%]{}]', '#|', '|#', ';'},
  [l.OPERATOR] = {
    ['('] = 1, [')'] = -1, ['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1
  },
  [l.COMMENT] = {['#|'] = 1, ['|#'] = -1, [';'] = l.fold_line_comments(';')}
}

return M
