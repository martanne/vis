-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Groovy LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'groovy'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local triple_sq_str = "'''" * (l.any - "'''")^0 * P("'''")^-1
local triple_dq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local regex_str = #P('/') * l.last_char_includes('=~|!<>+-*?&,:;([{') *
                  l.delimited_range('/', true)
local string = token(l.STRING, triple_sq_str + triple_dq_str + sq_str +
                               dq_str) +
               token(l.REGEX, regex_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'abstract', 'break', 'case', 'catch', 'continue', 'default', 'do', 'else',
  'extends', 'final', 'finally', 'for', 'if', 'implements', 'instanceof',
  'native', 'new', 'private', 'protected', 'public', 'return', 'static',
  'switch', 'synchronized', 'throw', 'throws', 'transient', 'try', 'volatile',
  'while', 'strictfp', 'package', 'import', 'as', 'assert', 'def', 'mixin',
  'property', 'test', 'using', 'in',
  'false', 'null', 'super', 'this', 'true', 'it'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'abs', 'any', 'append', 'asList', 'asWritable', 'call', 'collect',
  'compareTo', 'count', 'div', 'dump', 'each', 'eachByte', 'eachFile',
  'eachLine', 'every', 'find', 'findAll', 'flatten', 'getAt', 'getErr', 'getIn',
  'getOut', 'getText', 'grep', 'immutable', 'inject', 'inspect', 'intersect',
  'invokeMethods', 'isCase', 'join', 'leftShift', 'minus', 'multiply',
  'newInputStream', 'newOutputStream', 'newPrintWriter', 'newReader',
  'newWriter', 'next', 'plus', 'pop', 'power', 'previous', 'print', 'println',
  'push', 'putAt', 'read', 'readBytes', 'readLines', 'reverse', 'reverseEach',
  'round', 'size', 'sort', 'splitEachLine', 'step', 'subMap', 'times',
  'toInteger', 'toList', 'tokenize', 'upto', 'waitForOrKill', 'withPrintWriter',
  'withReader', 'withStream', 'withWriter', 'withWriterAppend', 'write',
  'writeLine'
})

-- Types.
local type = token(l.TYPE, word_match{
  'boolean', 'byte', 'char', 'class', 'double', 'float', 'int', 'interface',
  'long', 'short', 'void'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('=~|!<>+-/*?&.,:;()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'type', type},
  {'identifier', identifier},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
