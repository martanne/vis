-- Copyright 2013 Michael T. Richter. See LICENSE.
-- Copyright 2024 John Benediktsson <mrjbq7@gmail.com>
-- Factor lexer (http://factorcode.org).

-- At this time the lexer is usable, but not perfect.  Problems include:
--  * identifiers like (foo) get treated and coloured like stack declarations
--  * other as-yet unknown display bugs  :-)

local lexer = lexer
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local lex = lexer.new(...)

-- General building blocks.
local pre = lexer.upper^1
local post = pre
local opt_pre = pre^-1
local opt_post = opt_pre

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, P('#')^-1 * lexer.to_eol('!')))

-- Strings.
local dq1_str = opt_pre * lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, dq1_str))

-- Numbers.
-- Note that complex literals like C{ 1/3 27.3 } are not covered by this lexer.
-- The C{ ... } notation is treated as an operator--to be specific a
-- "constructor" (for want of a better term).
-- Also note that we cannot use lexer.number because numbers do not support the '+' prefix or
-- case-insensitive base-changing prefixes.
local hex_digits = lexer.xdigit^1
local binary = P('-')^-1 * '0b' * S('01')^1
local octal = P('-')^-1 * '0o' * R('07')^1
local decimal = P('-')^-1 * lexer.digit^1
local hexadecimal = P('-')^-1 * '0x' * hex_digits
local integer = binary + octal + hexadecimal + decimal
local ratio = decimal * '/' * decimal
local dfloat_component = decimal * '.' * decimal^-1
local hfloat_component = hexadecimal * ('.' * hex_digits^-1)^-1
local float = (dfloat_component * (S('eE') * decimal)^-1) + (hfloat_component * S('pP') * decimal) +
	(ratio * '.') + (P('-')^-1 * '1/0.') + ('0/0')
lex:add_rule('number', lex:tag(lexer.NUMBER, (float + ratio + integer)))

-- Keywords.
-- Things like NAN:, USE:, USING:, POSTPONE:, etc. are considered keywords, as are similar
-- words that end in #.
-- Patterns like <<WORD ... WORD>> are similarly considered to be "keywords" (for want of a
-- better term).
local colon_words = pre * S(':#') + S(':;')^1
local angle_words = (P('<')^1 * post) + (pre * P('>')^1)
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, (colon_words + angle_words)))

-- Operators.
-- The usual suspects like braces, brackets, angle brackets, parens, etc. are considered to be
-- operators. They may, however, have prefixes like C{ ... }.
local constructor_words = opt_pre * S('{[<') + S('}]>') + pre * '(' + ')'
local stack_declaration = lexer.range('(', ')')
local other_operators = S('+-*/<>')
lex:add_rule('operator',
	lex:tag(lexer.OPERATOR, (stack_declaration + constructor_words + other_operators)))

-- Identifiers.
-- Identifiers can be practically anything but whitespace.
local symbols = S('`~!@#$%^&*()_-+={[<>]}:;X,?/')
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, (lexer.alnum + symbols)^1))

lexer.property['factor.comment'] = '!'

return lex
