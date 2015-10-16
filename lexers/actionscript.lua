-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Actionscript LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'actionscript'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local ml_str = '<![CDATA[' * (l.any - ']]>')^0 * ']]>'
local string = token(l.STRING, sq_str + dq_str + ml_str)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.integer) * S('LlUuFf')^-2)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'break', 'continue', 'delete', 'do', 'else', 'for', 'function', 'if', 'in',
  'new', 'on', 'return', 'this', 'typeof', 'var', 'void', 'while', 'with',
  'NaN', 'Infinity', 'false', 'null', 'true', 'undefined',
  -- Reserved for future use.
  'abstract', 'case', 'catch', 'class', 'const', 'debugger', 'default',
  'export', 'extends', 'final', 'finally', 'goto', 'implements', 'import',
  'instanceof', 'interface', 'native', 'package', 'private', 'Void',
  'protected', 'public', 'dynamic', 'static', 'super', 'switch', 'synchonized',
  'throw', 'throws', 'transient', 'try', 'volatile'
})

-- Types.
local type = token(l.TYPE, word_match{
  'Array', 'Boolean', 'Color', 'Date', 'Function', 'Key', 'MovieClip', 'Math',
  'Mouse', 'Number', 'Object', 'Selection', 'Sound', 'String', 'XML', 'XMLNode',
  'XMLSocket',
  -- Reserved for future use.
  'boolean', 'byte', 'char', 'double', 'enum', 'float', 'int', 'long', 'short'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('=!<>+-/*%&|^~.,;?()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'type', type},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[{}]', '/%*', '%*/', '//', '<!%[CDATA%[', '%]%]>'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {
    ['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')
  },
  [l.STRING] = {['<![CDATA['] = 1, [']]>'] = -1}
}

return M
