-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- ASP LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'asp'}

-- Embedded in HTML.
local html = l.load('html')

-- Embedded VB.
local vb = l.load('vb')
local vb_start_rule = token('asp_tag', '<%' * P('=')^-1)
local vb_end_rule = token('asp_tag', '%>')
l.embed_lexer(html, vb, vb_start_rule, vb_end_rule)

-- Embedded VBScript.
local vbs = l.load('vbscript')
local script_element = word_match({'script'}, nil, html.case_insensitive_tags)
local vbs_start_rule = #(P('<') * script_element * (P(function(input, index)
  if input:find('^%s+language%s*=%s*(["\'])vbscript%1', index) or
     input:find('^%s+type%s*=%s*(["\'])text/vbscript%1', index) then
    return index
  end
end) + '>')) * html.embed_start_tag -- <script language="vbscript">
local vbs_end_rule = #('</' * script_element * l.space^0 * '>') *
                     html.embed_end_tag -- </script>
l.embed_lexer(html, vbs, vbs_start_rule, vbs_end_rule)

M._tokenstyles = {
  asp_tag = l.STYLE_EMBEDDED
}

local _foldsymbols = html._foldsymbols
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '<%%'
_foldsymbols._patterns[#_foldsymbols._patterns + 1] = '%%>'
_foldsymbols.asp_tag = {['<%'] = 1, ['%>'] = -1}
M._foldsymbols = _foldsymbols

return M
