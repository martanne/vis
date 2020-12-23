-- Copyright 2006-2020 Mitchell. See LICENSE.
-- Scheme LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('scheme')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  and begin case cond cond-expand define define-macro delay do else fluid-let if
  lambda let let* letrec or quasiquote quote set!
]]))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match[[
  abs acos angle append apply asin assoc assq assv atan car cdr caar cadr cdar
  cddr caaar caadr cadar caddr cdaar cdadr cddar cdddr
  call-with-current-continuation call-with-input-file call-with-output-file
  call-with-values call/cc catch ceiling char->integer char-downcase char-upcase
  close-input-port close-output-port cons cos current-input-port
  current-output-port delete-file display dynamic-wind eval exit exact->inexact
  exp expt file-or-directory-modify-seconds floor force for-each gcd gensym
  get-output-string getenv imag-part integer->char lcm length list list->string
  list->vector list-ref list-tail load log magnitude make-polar make-rectangular
  make-string make-vector map max member memq memv min modulo newline nil not
  number->string open-input-file open-input-string open-output-file
  open-output-string peek-char quotient read read-char read-line real-part
  remainder reverse reverse! round set-car! set-cdr! sin sqrt string
  string->list string->number string->symbol string-append string-copy
  string-fill! string-length string-ref string-set! substring symbol->string
  system tan truncate values vector vector->list vector-fill! vector-length
  vector-ref vector-set! with-input-from-file with-output-to-file write
  write-char
  boolean? char-alphabetic? char-ci<=? char-ci<? char-ci=? char-ci>=? char-ci>?
  char-lower-case? char-numeric? char-ready? char-upper-case? char-whitespace?
  char<=? char<? char=? char>=? char>? char? complex? eof-object? eq? equal?
  eqv? even? exact? file-exists? inexact? input-port? integer? list? negative?
  null? number? odd? output-port? pair? port? positive? procedure? rational?
  real? string-ci<=? string-ci<? string-ci=? string-ci>=? string-ci>? string<=?
  string<? string=? string>=? string>? string? symbol? vector? zero?
  #t #f
]]))

local word = (lexer.alpha + S('-!?')) * (lexer.alnum + S('-!?'))^0

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, word))

-- Strings.
local literal = (P("'") + '#' * S('\\bdox')) * lexer.word
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, literal + dq_str))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = lexer.range('#|', '|#')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, P('-')^-1 * lexer.digit^1 *
  (S('./') * lexer.digit^1)^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('<>=*/+-`@%:()')))

-- Entity.
lex:add_rule('entity', token('entity', '&' * word))
lex:add_style('entity', lexer.styles.variable)

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.COMMENT, '#|', '|#')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines(';'))

return lex
