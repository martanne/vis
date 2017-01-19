-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- XML LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = {_NAME = 'xml'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments and CDATA.
local comment = token(l.COMMENT, '<!--' * (l.any - '-->')^0 * P('-->')^-1)
local cdata = token('cdata', '<![CDATA[' * (l.any - ']]>')^0 * P(']]>')^-1)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"', false, true)
local string = #S('\'"') * l.last_char_includes('=') *
               token(l.STRING, sq_str + dq_str)

local in_tag = P(function(input, index)
  local before = input:sub(1, index - 1)
  local s, e = before:find('<[^>]-$'), before:find('>[^<]-$')
  if s and e then return s > e and index or nil end
  if s then return index end
  return input:find('^[^<]->', index) and index or nil
end)

-- Numbers.
local number = #l.digit * l.last_char_includes('=') *
               token(l.NUMBER, l.digit^1 * P('%')^-1) * in_tag

local alpha = R('az', 'AZ', '\127\255')
local word_char = l.alnum + S('_-:.??')
local identifier = (l.alpha + S('_-:.??')) * word_char^0
local namespace = token(l.OPERATOR, ':') * token('namespace', identifier)

-- Elements.
local element = token('element', '<' * P('/')^-1 * identifier) * namespace^-1

-- Attributes.
local attribute = token('attribute', identifier) * namespace^-1 *
                  #(l.space^0 * '=')

-- Closing tags.
local close_tag = token('element', P('/')^-1 * '>')

-- Equals.
local equals = token(l.OPERATOR, '=') * in_tag

-- Entities.
local entity = token('entity', '&' * word_match{
  'lt', 'gt', 'amp', 'apos', 'quot'
} * ';')

-- Doctypes and other markup tags.
local doctype = token('doctype', P('<!DOCTYPE')) * ws *
                token('doctype', identifier) * (ws * identifier)^-1 *
                (1 - P('>'))^0 * token('doctype', '>')

-- Processing instructions.
local proc_insn = token('proc_insn', P('<?') * (1 - P('?>'))^0 * P('?>')^-1)

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'cdata', cdata},
  {'doctype', doctype},
  {'proc_insn', proc_insn},
  {'element', element},
  {'close_tag', close_tag},
  {'attribute', attribute},
  {'equals', equals},
  {'string', string},
  {'number', number},
  {'entity', entity},
}

M._tokenstyles = {
  element = l.STYLE_KEYWORD,
  namespace = l.STYLE_CLASS,
  attribute = l.STYLE_TYPE,
  cdata = l.STYLE_COMMENT,
  entity = l.STYLE_OPERATOR,
  doctype = l.STYLE_COMMENT,
  proc_insn = l.STYLE_COMMENT,
  --markup = l.STYLE_COMMENT
}

M._foldsymbols = {
  _patterns = {'</?', '/>', '<!%-%-', '%-%->', '<!%[CDATA%[', '%]%]>'},
  element = {['<'] = 1, ['/>'] = -1, ['</'] = -1},
  [l.COMMENT] = {['<!--'] = 1, ['-->'] = -1},
  cdata = {['<![CDATA['] = 1, [']]>'] = -1}
}

return M
