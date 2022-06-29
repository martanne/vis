-- Copyright 2006-2022 Mitchell. See LICENSE.
-- VisualBasic LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('vb', {case_insensitive_fold_points = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
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
}, true)))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match(
  'Boolean Byte Char Date Decimal Double Long Object Short Single String', true)))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol("'" + word_match('rem', true))))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true, false)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('LlUuFf')^-2))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=><+-*^&:.,_()')))

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

return lex
