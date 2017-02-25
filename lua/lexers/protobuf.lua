-- Copyright 2016-2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- Protocol Buffer IDL LPeg lexer.
-- <https://developers.google.com/protocol-buffers/>

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'protobuf'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = P('L')^-1 * l.delimited_range("'", true)
local dq_str = P('L')^-1 * l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'contained', 'syntax', 'import', 'option', 'package', 'message', 'group',
  'oneof', 'optional', 'required', 'repeated', 'default', 'extend',
  'extensions', 'to', 'max', 'reserved', 'service', 'rpc', 'returns'
})

-- Types.
local type = token(l.TYPE, word_match{
  'int32', 'int64', 'uint32', 'uint64', 'sint32', 'sint64', 'fixed32',
  'fixed64', 'sfixed32', 'sfixed64', 'float', 'double', 'bool', 'string',
  'bytes', 'enum', 'true', 'false'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('<>=|;,.()[]{}'))

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

return M
