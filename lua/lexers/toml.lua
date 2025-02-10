-- Copyright 2015-2025 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- TOML LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {fold_by_indentation = true})

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lexer.word_match('true false')))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('=+-,.{}[]()')))

-- Datetime.
local year = lexer.digit * lexer.digit * lexer.digit * lexer.digit
local month = lexer.digit * lexer.digit^-1
local day = lexer.digit * lexer.digit^-1
local date = year * '-' * month * '-' * day
local hours = lexer.digit * lexer.digit^-1
local minutes = lexer.digit * lexer.digit
local seconds = lexer.digit * lexer.digit
local fraction = '.' * lexer.digit^0
local time = hours * ':' * minutes * ':' * seconds * fraction^-1
local zone = 'Z' + S(' \t')^0 * S('-+') * hours * (':' * minutes)^-1
lex:add_rule('datetime', lex:tag(lexer.NUMBER .. '.timestamp', date * (S('tT \t') * time * zone^-1)))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

lexer.property['scintillua.comment'] = '#'

return lex
