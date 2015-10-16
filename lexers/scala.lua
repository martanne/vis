-- Copyright 2006-2013 JMS. See LICENSE.
-- Scala LPeg Lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'scala'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local symbol = "'" * l.word
local dq_str = l.delimited_range('"', true)
local tq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local string = token(l.STRING, tq_str + symbol + dq_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('LlFfDd')^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'abstract', 'case', 'catch', 'class', 'def', 'do', 'else', 'extends', 'false',
  'final', 'finally', 'for', 'forSome', 'if', 'implicit', 'import', 'lazy',
  'match', 'new', 'null', 'object', 'override', 'package', 'private',
  'protected', 'return', 'sealed', 'super', 'this', 'throw', 'trait', 'try',
  'true', 'type', 'val', 'var', 'while', 'with', 'yield'
})

-- Types.
local type = token(l.TYPE, word_match{
  'Array', 'Boolean', 'Buffer', 'Byte', 'Char', 'Collection', 'Double', 'Float',
  'Int', 'Iterator', 'LinkedList', 'List', 'Long', 'Map', 'None', 'Option',
  'Set', 'Short', 'SortedMap', 'SortedSet', 'String', 'TreeMap', 'TreeSet'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}'))

-- Functions.
local func = token(l.FUNCTION, l.word) * #P('(')

-- Classes.
local class_sequence = token(l.KEYWORD, P('class')) * ws^1 *
                       token(l.CLASS, l.word)

M._rules = {
  {'whitespace', ws},
  {'class', class_sequence},
  {'keyword', keyword},
  {'type', type},
  {'function', func},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
