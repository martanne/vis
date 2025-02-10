-- Copyright 2016-2025 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- Dockerfile LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(..., {fold_by_indentation = true})

-- Keywords.
local keyword = lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD))
lex:add_rule('keyword', keyword)

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Variable.
lex:add_rule('variable',
-B('\\') * lex:tag(lexer.OPERATOR, '$' * P('{')^-1) * lex:tag(lexer.VARIABLE, lexer.word))

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"')
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('\\[],=:{}')))

local bash = lexer.load('bash')
local start_rule = #P('RUN') * keyword * bash:get_rule('whitespace')
local end_rule = -B('\\') * #lexer.newline * lex:get_rule('whitespace')
lex:embed(bash, start_rule, end_rule)

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'ADD', 'ARG', 'CMD', 'COPY', 'ENTRYPOINT', 'ENV', 'EXPOSE', 'FROM', 'LABEL', 'MAINTAINER',
	'ONBUILD', 'RUN', 'STOPSIGNAL', 'USER', 'VOLUME', 'WORKDIR'
})

lexer.property['scintillua.comment'] = '#'

return lex
