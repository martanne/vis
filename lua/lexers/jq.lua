-- Copyright 2006-2022 Mitchell. See LICENSE.
-- jq 1.6 Lua lexer -- https://stedolan.github.io/jq/wiki
-- Anonymously contributed.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('jq')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- keywords not listed by jq's "builtins", minus operators 'and' and 'or', plus the '?' shorthand
  'as', 'break', 'catch', 'def', 'elif', 'else', 'end', 'foreach', 'if', 'import', 'include',
  'label', 'module', 'reduce', 'then', 'try'
} + '?'))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  -- jq 1.6 built-in functions (SQL in upper caisse)
  'acos', 'acosh', 'add', 'all', 'any', 'arrays', 'ascii_downcase', 'ascii_upcase', 'asin', 'asinh',
  'atan', 'atan2', 'atanh', 'booleans', 'bsearch', 'builtins', 'capture', 'cbrt', 'ceil',
  'combinations', 'contains', 'copysign', 'cos', 'cosh', 'debug', 'del', 'delpaths', 'drem',
  'empty', 'endswith', 'env', 'erf', 'erfc', 'error', 'exp', 'exp10', 'exp2', 'explode', 'expm1',
  'fabs', 'fdim', 'finites', 'first', 'flatten', 'floor', 'fma', 'fmax', 'fmin', 'fmod', 'format',
  'frexp', 'from_entries', 'fromdate', 'fromdateiso8601', 'fromjson', 'fromstream', 'gamma',
  'get_jq_origin', 'get_prog_origin', 'get_search_list', 'getpath', 'gmtime', 'group_by', 'gsub',
  'halt', 'halt_error', 'has', 'hypot', 'implode', 'IN', 'in', 'INDEX', 'index', 'indices',
  'infinite', 'input', 'input_filename', 'input_line_number', 'inputs', 'inside', 'isempty',
  'isfinite', 'isinfinite', 'isnan', 'isnormal', 'iterables', 'j0', 'j1', 'jn', 'JOIN', 'join',
  'keys', 'keys_unsorted', 'last', 'ldexp', 'leaf_paths', 'length', 'lgamma', 'lgamma_r', 'limit',
  'localtime', 'log', 'log10', 'log1p', 'log2', 'logb', 'ltrimstr', 'map', 'map_values', 'match',
  'max', 'max_by', 'min', 'min_by', 'mktime', 'modf', 'modulemeta', 'nan', 'nearbyint', 'nextafter',
  'nexttoward', 'normals', 'not', 'now', 'nth', 'nulls', 'numbers', 'objects', 'path', 'paths',
  'pow', 'pow10', 'range', 'recurse', 'recurse_down', 'remainder', 'repeat', 'reverse', 'rindex',
  'rint', 'round', 'rtrimstr', 'scalars', 'scalars_or_empty', 'scalb', 'scalbln', 'scan', 'select',
  'setpath', 'significand', 'sin', 'sinh', 'sort', 'sort_by', 'split', 'splits', 'sqrt',
  'startswith', 'stderr', 'strflocaltime', 'strftime', 'strings', 'strptime', 'sub', 'tan', 'tanh',
  'test', 'tgamma', 'to_entries', 'todate', 'todateiso8601', 'tojson', 'tonumber', 'tostream',
  'tostring', 'transpose', 'trunc', 'truncate_stream', 'type', 'unique', 'unique_by', 'until',
  'utf8bytelength', 'values', 'walk', 'while', 'with_entries', 'y0', 'y1', 'yn'
}))

-- Strings.
local string = token(lexer.STRING, lexer.range('"', true))
local literal = token(lexer.STRING, word_match('null false true'))
lex:add_rule('string', string + literal)

-- Operators.
-- 'not' isn't an operator but a function (filter)
lex:add_rule('operator', token(lexer.OPERATOR,
  P('.[]') + '?//' + '//=' + 'and' + '[]' + '//' + '==' + '!=' + '>=' + '<=' + '|=' + '+=' + '-=' +
    '*=' + '/=' + '%=' + 'or' + S('=+-*/%<>()[]{}.,') + '|' + ';'))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Formats.
lex:add_rule('format',
  token('format', '@' * word_match('text json html uri csv tsv sh base64 base64d')))
lex:add_style('format', lexer.styles.constant)

-- Variables.
lex:add_rule('sysvar', token('sysvar', '$' * word_match('ENV  ORIGIN  __loc__')))
lex:add_style('sysvar', lexer.styles.constant .. {bold = true})
lex:add_rule('variable', token(lexer.VARIABLE, '$' * lexer.word))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
