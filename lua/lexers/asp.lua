-- Copyright 2006-2022 Mitchell. See LICENSE.
-- ASP LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local html = lexer.load('html')
local lex = lexer.new('asp', {inherit = html}) -- proxy for HTML

-- Embedded VB.
local vb = lexer.load('vb')
local vb_start_rule = token('asp_tag', '<%' * P('=')^-1)
local vb_end_rule = token('asp_tag', '%>')
lex:embed(vb, vb_start_rule, vb_end_rule)
lex:add_style('asp_tag', lexer.styles.embedded)

-- Embedded VBScript.
local vbs = lexer.load('vb', 'vbscript')
local script_element = word_match('script', true)
local vbs_start_rule = #(P('<') * script_element * (P(function(input, index)
  if input:find('^%s+language%s*=%s*(["\'])vbscript%1', index) or
    input:find('^%s+type%s*=%s*(["\'])text/vbscript%1', index) then return index end
end) + '>')) * html.embed_start_tag -- <script language="vbscript">
local vbs_end_rule = #('</' * script_element * lexer.space^0 * '>') * html.embed_end_tag -- </script>
lex:embed(vbs, vbs_start_rule, vbs_end_rule)

-- Fold points.
lex:add_fold_point('asp_tag', '<%', '%>')

return lex
