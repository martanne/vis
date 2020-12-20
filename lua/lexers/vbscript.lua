-- Copyright 2006-2020 Mitchell. See LICENSE.
-- VisualBasic LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('vbscript')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match([[
  -- Control.
  If Then Else ElseIf While Wend For To Each In Step Case Select Return Continue
  Do Until Loop Next With Exit
  -- Operators.
  Mod And Not Or Xor Is
  -- Storage types.
  Call Class Const Dim ReDim Preserve Function Sub Property End Set Let Get New
  Randomize Option Explicit On Error Execute
  -- Storage modifiers.
  Private Public Default
  -- Constants.
  Empty False Nothing Null True
]], true)))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match([[
  Boolean Byte Char Date Decimal Double Long Object Short Single String
]], true)))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT,
  lexer.to_eol("'" + word_match([[rem]], true))))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true, false)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number * S('LlUuFf')^-2))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=><+-*^&:.,_()')))

return lex
