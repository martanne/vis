-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Io LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'io_lang'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = (P('#') + '//') * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local tq_str = '"""' * (l.any - '"""')^0 * P('"""')^-1
local string = token(l.STRING, tq_str + sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'block', 'method', 'while', 'foreach', 'if', 'else', 'do', 'super', 'self',
  'clone', 'proto', 'setSlot', 'hasSlot', 'type', 'write', 'print', 'forward'
})

-- Types.
local type = token(l.TYPE, word_match{
  'Block', 'Buffer', 'CFunction', 'Date', 'Duration', 'File', 'Future', 'List',
  'LinkedList', 'Map', 'Nop', 'Message', 'Nil', 'Number', 'Object', 'String',
  'WeakLink'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('`~@$%^&*-+/=\\<>?.,:;()[]{}'))

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
  _patterns = {'[%(%)]', '/%*', '%*/', '#', '//'},
  [l.OPERATOR] = {['('] = 1, [')'] = -1},
  [l.COMMENT] = {
    ['/*'] = 1, ['*/'] = -1, ['#'] = l.fold_line_comments('#'),
    ['//'] = l.fold_line_comments('//')
  }
}

return M
