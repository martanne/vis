-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Forth LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'forth'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = S('|\\') * l.nonnewline^0
local block_comment = '(' * (l.any - ')')^0 * P(')')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local c_str = 'c' * l.delimited_range('"', true, true)
local s_str = 's' * l.delimited_range('"', true, true)
local s_bs_str = 's\\' * l.delimited_range('"', true, false)
local dot_str = '.' * l.delimited_range('"', true, true)
local dot_paren_str = '.' * l.delimited_range('()', true, true, false)
local abort_str = 'abort' * l.delimited_range('"', true, true)
local string = token(
  l.STRING,
  c_str + s_str + s_bs_str + dot_str + dot_paren_str + abort_str
)

-- Numbers.
local number = token(l.NUMBER, P('-')^-1 * l.digit^1 * (S('./') * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  '#>', '#s', '*/', '*/mod', '+loop', ',', '.', '.r', '/mod', '0<', '0<>', 
  '0>', '0=', '1+', '1-', '2!', '2*', '2/', '2>r', '2@', '2drop', '2dup',
  '2over', '2r>', '2r@', '2swap', ':noname', '<#', '<>', '>body', '>in', 
  '>number', '>r', '?do','?dup', '@', 'abort', 'abs', 'accept', 'action-of',
  'again', 'align', 'aligned', 'allot', 'and', 'base', 'begin', 'bl',
  'buffer:', 'c!', 'c,', 'c@', 'case', 'cell+', 'cells', 'char', 'char+',
  'chars', 'compile,', 'constant', 'count', 'cr', 'create', 'decimal', 'defer',
  'defer!', 'defer@', 'depth', 'do', 'does>', 'drop', 'dup', 'else', 'emit',
  'endcase', 'endof', 'environment?', 'erase', 'evaluate', 'execute', 'exit',
  'false', 'fill', 'find', 'fm/mod', 'here', 'hex', 'hold', 'holds', 'i', 'if',
  'immediate', 'invert', 'is', 'j', 'key', 'leave', 'literal', 'loop', 
  'lshift', 'm*', 'marker', 'max', 'min', 'mod', 'move', 'negate', 'nip', 'of',
  'or', 'over', 'pad', 'parse', 'parse-name', 'pick', 'postpone', 'quit', 'r>',
  'r@', 'recurse', 'refill', 'restore-input', 'roll', 'rot', 'rshift', 's>d',
  'save-input', 'sign', 'sm/rem', 'source', 'source-id', 'space', 'spaces',
  'state', 'swap', 'to', 'then', 'true', 'tuck', 'type', 'u.', 'u.r', 'u>',
  'u<', 'um*', 'um/mod', 'unloop', 'until', 'unused', 'value', 'variable',
  'while', 'within', 'word', 'xor', '[\']', '[char]', '[compile]'
}, '><-@!?+,=[].\'', true))

-- Identifiers.
local identifier = token(l.IDENTIFIER, (l.alnum + S('+-*=<>.?/\'%,_$#'))^1)

-- Operators.
local operator = token(l.OPERATOR, S(':;<>+*-/[]#'))

M._rules = {
  {'whitespace', ws},
  {'string', string},
  {'keyword', keyword},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
