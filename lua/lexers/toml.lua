-- Copyright 2015-2022 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- TOML LPeg lexer.

local lexer = require("lexer")
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('toml', {fold_by_indentation = true})

-- Whitespace
lex:add_rule('whitespace', token(lexer.WHITESPACE, S(' \t')^1 + lexer.newline^1))

-- kewwords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match('true false')))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=+-,.{}[]()')))

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
local T = S(' \t')^1 + S('tT')
local zone = 'Z' + S(' \t')^0 * S('-+') * hours * (':' * minutes)^-1
lex:add_rule('datetime', token('timestamp', date * (T * time * zone^-1)))
lex:add_style('timestamp', lexer.styles.number)

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

return lex
