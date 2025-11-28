-- Copyright 2006-2025 Mitchell. See LICENSE.
-- Haskell LPeg lexer.
-- Modified by Alex Suraci.
-- Migrated by Samuel Marquis.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {fold_by_indentation = true})

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types & type constructors.
local word = (lexer.alnum + S("._'#"))^0
local op = lexer.punct - S('()[]{}')
lex:add_rule('type', lex:tag(lexer.TYPE, (lexer.upper * word) + (':' * (op^1 - ':'))))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, (lexer.alpha + '_') * word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"')
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Comments.
local line_comment = lexer.to_eol('--', true)
local block_comment = lexer.range('{-', '-}')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, '..' + op))

lexer.property['scintillua.comment'] = '--'

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'case', 'class', 'data', 'default', 'deriving', 'do', 'else', 'if', 'import', 'in', 'infix',
	'infixl', 'infixr', 'instance', 'let', 'module', 'newtype', 'of', 'then', 'type', 'where', '_',
	'as', 'qualified', 'hiding'
})

return lex
