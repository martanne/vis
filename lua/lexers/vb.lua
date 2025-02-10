-- Copyright 2006-2025 Mitchell. See LICENSE.
-- VisualBasic LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {case_insensitive_fold_points = true})

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD, true)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE, true)))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol("'" + lexer.word_match('rem', true))))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', lex:tag(lexer.STRING, lexer.range('"', true, false)))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number * S('LlUuFf')^-2))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('=><+-*^&:.,_()')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'If', 'End If')
lex:add_fold_point(lexer.KEYWORD, 'Select', 'End Select')
lex:add_fold_point(lexer.KEYWORD, 'For', 'Next')
lex:add_fold_point(lexer.KEYWORD, 'While', 'End While')
lex:add_fold_point(lexer.KEYWORD, 'While', 'Wend')
lex:add_fold_point(lexer.KEYWORD, 'Do', 'Loop')
lex:add_fold_point(lexer.KEYWORD, 'With', 'End With')
lex:add_fold_point(lexer.KEYWORD, 'Sub', 'End Sub')
lex:add_fold_point(lexer.KEYWORD, 'Function', 'End Function')
lex:add_fold_point(lexer.KEYWORD, 'Property', 'End Property')
lex:add_fold_point(lexer.KEYWORD, 'Module', 'End Module')
lex:add_fold_point(lexer.KEYWORD, 'Class', 'End Class')
lex:add_fold_point(lexer.KEYWORD, 'Try', 'End Try')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	-- Control.
	'If', 'Then', 'Else', 'ElseIf', 'While', 'Wend', 'For', 'To', 'Each', 'In', 'Step', 'Case',
	'Select', 'Return', 'Continue', 'Do', 'Until', 'Loop', 'Next', 'With', 'Exit',
	-- Operators.
	'Mod', 'And', 'Not', 'Or', 'Xor', 'Is',
	-- Storage types.
	'Call', 'Class', 'Const', 'Dim', 'ReDim', 'Preserve', 'Function', 'Sub', 'Property', 'End', 'Set',
	'Let', 'Get', 'New', 'Randomize', 'Option', 'Explicit', 'On', 'Error', 'Execute', 'Module',
	-- Storage modifiers.
	'Private', 'Public', 'Default',
	-- Constants.
	'Empty', 'False', 'Nothing', 'Null', 'True'
})

lex:set_word_list(lexer.TYPE, {
	'Boolean', 'Byte', 'Char', 'Date', 'Decimal', 'Double', 'Long', 'Object', 'Short', 'Single',
	'String'
})

lexer.property['scintillua.comment'] = "'"

return lex
