-- Copyright 2006-2020 Mitchell. See LICENSE.
-- jq 1.6 Lua lexer -- https://stedolan.github.io/jq/wiki
-- Anonymously contributed.

-- Basic definitions.
local lexer = require('lexer')
local P = lpeg.P

-- Define the lexer's tokens.
local whitespace = lexer.token(lexer.WHITESPACE, lexer.space^1)
local comment = lexer.token(lexer.COMMENT, '#' * lexer.nonnewline^0)
local string = lexer.token(lexer.STRING, lexer.range('"', true))
local literal = lexer.token(lexer.STRING, P('null')+P('false')+P('true'))
local number = lexer.token(lexer.NUMBER, lexer.float + lexer.integer)
local keyword = lexer.token(lexer.KEYWORD, lexer.word_match[[
  -- keywords not listed by jq's "builtins", minus
  -- operators 'and' and 'or', plus the '?' shorthand
  as break catch def elif else end foreach if import include label module reduce
  then try
]] + P('?'))
local funxion = lexer.token(lexer.FUNCTION, lexer.word_match[[
  -- jq 1.6 built-in functions (SQL in upper caisse)
  acos acosh add all any arrays ascii_downcase ascii_upcase asin asinh atan
  atan2 atanh booleans bsearch builtins capture cbrt ceil combinations contains
  copysign cos cosh debug del delpaths drem empty endswith env erf erfc error
  exp exp10 exp2 explode expm1 fabs fdim finites first flatten floor fma fmax
  fmin fmod format frexp from_entries fromdate fromdateiso8601 fromjson
  fromstream gamma get_jq_origin get_prog_origin get_search_list getpath gmtime
  group_by gsub halt halt_error has hypot implode IN in INDEX index indices
  infinite input input_filename input_line_number inputs inside isempty isfinite
  isinfinite isnan isnormal iterables j0 j1 jn JOIN join keys keys_unsorted last
  ldexp leaf_paths length lgamma lgamma_r limit localtime log log10 log1p log2
  logb ltrimstr map map_values match max max_by min min_by mktime modf
  modulemeta nan nearbyint nextafter nexttoward normals not now nth nulls
  numbers objects path paths pow pow10 range recurse recurse_down remainder
  repeat reverse rindex rint round rtrimstr scalars scalars_or_empty scalb
  scalbln scan select setpath significand sin sinh sort sort_by split splits
  sqrt startswith stderr strflocaltime strftime strings strptime sub tan tanh
  test tgamma to_entries todate todateiso8601 tojson tonumber tostream tostring
  transpose trunc truncate_stream type unique unique_by until utf8bytelength
  values walk while with_entries y0 y1 yn
]])
-- 'not' isn't an operator but a function (filter)
local op = P('.[]') + P('?//') + P('//=') + P('and') + P('[]') + P('//') +
  P('==') + P('!=') + P('>=') + P('<=') + P('|=') + P('+=') + P('-=') +
  P('*=') + P('/=') + P('%=') + P('or') + lpeg.S('=+-*/%<>()[]{}') +
  lpeg.S('.,') + P('|') + P(';')
local operator = lexer.token(lexer.OPERATOR, op)
local format = lexer.token('format', P('@') * lexer.word_match[[
    text  json  html  uri  csv  tsv  sh  base64  base64d
]])
local sysvar = lexer.token('sysvar', P('$') * lexer.word_match[[
    ENV  ORIGIN  __loc__
]])
local variable = lexer.token(lexer.VARIABLE, P('$') * lexer.word)
local identifier = lexer.token(lexer.IDENTIFIER, lexer.word)

-- Specify the lexer's name.
local lex = lexer.new('jq')

-- Define the rules for the lexer's grammar.
lex:add_rule('whitespace', whitespace)
lex:add_rule('comment', comment)
lex:add_rule('string', string + literal)
lex:add_rule('number', number)
lex:add_rule('keyword', keyword)
lex:add_rule('function', funxion)
lex:add_rule('operator', operator)
lex:add_rule('identifier', identifier)
lex:add_rule('format', format)
lex:add_rule('sysvar', sysvar)
lex:add_rule('variable', variable)

-- Define styles for the lexer's custom tokens.
lex:add_style('format', lexer.STYLE_CONSTANT)
lex:add_style('sysvar', lexer.STYLE_CONSTANT .. {bold = true})

-- Specify how the lexer folds code.
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.KEYWORD, 'if', 'end')

return lex
