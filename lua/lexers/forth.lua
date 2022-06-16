-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Forth LPeg lexer.
-- Contributions from Joseph Eib.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('forth')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Strings.
local c_str = 'c' * lexer.range('"', true, false)
local s_str = 's' * lexer.range('"', true, false)
local s_bs_str = 's\\' * lexer.range('"', true)
local dot_str = '.' * lexer.range('"', true, false)
local dot_paren_str = '.' * lexer.range('(', ')', true)
local abort_str = 'abort' * lexer.range('"', true, false)
lex:add_rule('string',
  token(lexer.STRING, c_str + s_str + s_bs_str + dot_str + dot_paren_str + abort_str))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  '#>', '#s', '*/', '*/mod', '+loop', ',', '.', '.r', '/mod', '0<', '0<>', '0>', '0=', '1+', '1-',
  '2!', '2*', '2/', '2>r', '2@', '2drop', '2dup', '2over', '2r>', '2r@', '2swap', ':noname', '<#',
  '<>', '>body', '>in', '>number', '>r', '?do', '?dup', '@', 'abort', 'abs', 'accept', 'action-of',
  'again', 'align', 'aligned', 'allot', 'and', 'base', 'begin', 'bl', 'buffer:', 'c!', 'c,', 'c@',
  'case', 'cell+', 'cells', 'char', 'char+', 'chars', 'compile,', 'constant,', 'count', 'cr',
  'create', 'decimal', 'defer', 'defer!', 'defer@', 'depth', 'do', 'does>', 'drop', 'dup', 'else',
  'emit', 'endcase', 'endof', 'environment?', 'erase', 'evaluate', 'execute', 'exit', 'false',
  'fill', 'find', 'fm/mod', 'here', 'hex', 'hold', 'holds', 'i', 'if', 'immediate', 'invert', 'is',
  'j', 'key', 'leave', 'literal', 'loop', 'lshift', 'm*', 'marker', 'max', 'min', 'mod', 'move',
  'negate', 'nip', 'of', 'or', 'over', 'pad', 'parse', 'parse-name', 'pick', 'postpone', 'quit',
  'r>', 'r@', 'recurse', 'refill', 'restore-input', 'roll', 'rot', 'rshift', 's>d', 'save-input',
  'sign', 'sm/rem', 'source', 'source-id', 'space', 'spaces', 'state', 'swap', 'to', 'then', 'true',
  'tuck', 'type', 'u.', 'u.r', 'u>', 'u<', 'um*', 'um/mod', 'unloop', 'until', 'unused', 'value',
  'variable', 'while', 'within', 'word', 'xor', "[']", '[char]', '[compile]'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alnum + S('+-*=<>.?/\'%,_$#'))^1))

-- Comments.
local line_comment = lexer.to_eol(S('|\\'))
local block_comment = lexer.range('(', ')')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, P('-')^-1 * lexer.digit^1 * (S('./') * lexer.digit^1)^-1))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S(':;<>+*-/[]#')))

return lex
