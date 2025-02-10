-- Copyright 2006-2025 Mitchell. See LICENSE.
-- ASP LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local html = lexer.load('html')
local lex = lexer.new(..., {inherit = html}) -- proxy for HTML

-- Embedded VB.
local vb = lexer.load('vb')
local vb_start_rule = lex:tag(lexer.PREPROCESSOR, '<%' * P('=')^-1)
local vb_end_rule = lex:tag(lexer.PREPROCESSOR, '%>')
lex:embed(vb, vb_start_rule, vb_end_rule)

-- Embedded VBScript.
local vbs = lexer.load('vb', 'vbscript')
local script_element = lexer.word_match('script', true)
local vbs_start_rule = #('<' * script_element * (P(function(input, index)
	if input:find('^%s+language%s*=%s*(["\'])vbscript%1', index) or
		input:find('^%s+type%s*=%s*(["\'])text/vbscript%1', index) then return true end
end) + '>')) * html.embed_start_tag -- <script language="vbscript">
local vbs_end_rule = #('</' * script_element * '>') * html.embed_end_tag -- </script>
lex:embed(vbs, vbs_start_rule, vbs_end_rule)

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, '<%', '%>')

lexer.property['scintillua.comment'] = '<!--|-->'

return lex
