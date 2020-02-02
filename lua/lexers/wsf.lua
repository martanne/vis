-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- WSF LPeg lexer (based on XML).
-- Contributed by Jeff Stone.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S, V = lpeg.P, lpeg.R, lpeg.S, lpeg.V

local M = {_NAME = 'wsf'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '<!--' * (l.any - '-->')^0 * P('-->')^-1)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"', false, true)
local string = #S('\'"') * l.last_char_includes('=') *
               token(l.STRING, sq_str + dq_str)

local in_tag = #P((1 - S'><')^0 * '>')

-- Numbers.
local number = #l.digit * l.last_char_includes('=') *
               token(l.NUMBER, l.digit^1 * P('%')^-1) * in_tag

local alpha = R('az', 'AZ', '\127\255')
local word_char = l.alnum + S('_-:.??')
local identifier = (l.alpha + S('_-:.??')) * word_char^0

-- Elements.
local element = token('element', '<' * P('/')^-1 * identifier)

-- Attributes.
local attribute = token('attribute', identifier) * #(l.space^0 * '=')

-- Closing tags.
local tag_close = token('element', P('/')^-1 * '>')

-- Equals.
local equals = token(l.OPERATOR, '=') * in_tag

-- Entities.
local entity = token('entity', '&' * word_match{
  'lt', 'gt', 'amp', 'apos', 'quot'
} * ';')

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'element', element},
  {'tag_close', tag_close},
  {'attribute', attribute},
  {'equals', equals},
  {'string', string},
  {'number', number},
  {'entity', entity}
}

M._tokenstyles = {
  element = l.STYLE_KEYWORD,
  attribute = l.STYLE_TYPE,
  entity = l.STYLE_OPERATOR
}

M._foldsymbols = {
  _patterns = {'</?', '/>', '<!%-%-', '%-%->'},
  element = {['<'] = 1, ['/>'] = -1, ['</'] = -1},
  [l.COMMENT] = {['<!--'] = 1, ['-->'] = -1},
}

-- Finally, add JavaScript and VBScript as embedded languages

-- Tags that start embedded languages.
M.embed_start_tag = element *
                    (ws^1 * attribute * ws^0 * equals * ws^0 * string)^0 *
                    ws^0 * tag_close
M.embed_end_tag = element * tag_close

-- Embedded JavaScript.
local js = l.load('javascript')
local js_start_rule = #(P('<script') * (P(function(input, index)
  if input:find('^%s+language%s*=%s*(["\'])[jJ][ava]*[sS]cript%1', index) then
    return index
  end
end) + '>')) * M.embed_start_tag -- <script language="javascript">
local js_end_rule = #('</script' * ws^0 * '>') * M.embed_end_tag -- </script>
l.embed_lexer(M, js, js_start_rule, js_end_rule)

-- Embedded VBScript.
local vbs = l.load('vbscript')
local vbs_start_rule = #(P('<script') * (P(function(input, index)
  if input:find('^%s+language%s*=%s*(["\'])[vV][bB][sS]cript%1', index) then
    return index
  end
end) + '>')) * M.embed_start_tag -- <script language="vbscript">
local vbs_end_rule = #('</script' * ws^0 * '>') * M.embed_end_tag -- </script>
l.embed_lexer(M, vbs, vbs_start_rule, vbs_end_rule)

return M
