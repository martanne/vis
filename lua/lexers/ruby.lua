-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Ruby LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('ruby')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'BEGIN', 'END', 'alias', 'and', 'begin', 'break', 'case', 'class', 'def', 'defined?', 'do',
  'else', 'elsif', 'end', 'ensure', 'false', 'for', 'if', 'in', 'module', 'next', 'nil', 'not',
  'or', 'redo', 'rescue', 'retry', 'return', 'self', 'super', 'then', 'true', 'undef', 'unless',
  'until', 'when', 'while', 'yield', '__FILE__', '__LINE__'
}))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'at_exit', 'autoload', 'binding', 'caller', 'catch', 'chop', 'chop!', 'chomp', 'chomp!', 'eval',
  'exec', 'exit', 'exit!', 'extend', 'fail', 'fork', 'format', 'gets', 'global_variables', 'gsub',
  'gsub!', 'include', 'iterator?', 'lambda', 'load', 'local_variables', 'loop', 'module_function',
  'open', 'p', 'print', 'printf', 'proc', 'putc', 'puts', 'raise', 'rand', 'readline', 'readlines',
  'require', 'require_relative', 'select', 'sleep', 'split', 'sprintf', 'srand', 'sub', 'sub!',
  'syscall', 'system', 'test', 'trace_var', 'trap', 'untrace_var'
}) * -S('.:|'))

-- Identifiers.
local word_char = lexer.alnum + S('_!?')
local word = (lexer.alpha + '_') * word_char^0
lex:add_rule('identifier', token(lexer.IDENTIFIER, word))

-- Comments.
local line_comment = lexer.to_eol('#', true)
local block_comment = lexer.range(lexer.starts_line('=begin'), lexer.starts_line('=end'))
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Strings.
local delimiter_matches = {['('] = ')', ['['] = ']', ['{'] = '}'}
local literal_delimited = P(function(input, index)
  local delimiter = input:sub(index, index)
  if not delimiter:find('[%w\r\n\f\t ]') then -- only non alpha-numerics
    local match_pos, patt
    if delimiter_matches[delimiter] then
      -- Handle nested delimiter/matches in strings.
      local s, e = delimiter, delimiter_matches[delimiter]
      patt = lexer.range(s, e, false, true, true)
    else
      patt = lexer.range(delimiter)
    end
    match_pos = lpeg.match(patt, input, index)
    return match_pos or #input + 1
  end
end)

local cmd_str = lexer.range('`')
local lit_cmd = '%x' * literal_delimited
local lit_array = '%w' * literal_delimited
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local lit_str = '%' * S('qQ')^-1 * literal_delimited
local heredoc = '<<' * P(function(input, index)
  local s, e, indented, _, delimiter = input:find('([%-~]?)(["`]?)([%a_][%w_]*)%2[\n\r\f;]+', index)
  if s == index and delimiter then
    local end_heredoc = (#indented > 0 and '[\n\r\f]+ *' or '[\n\r\f]+')
    e = select(2, input:find(end_heredoc .. delimiter, e))
    return e and e + 1 or #input + 1
  end
end)
local string = token(lexer.STRING, (sq_str + dq_str + lit_str + heredoc + cmd_str + lit_cmd +
  lit_array) * S('f')^-1)
-- TODO: regex_str fails with `obj.method /patt/` syntax.
local regex_str =
  #P('/') * lexer.last_char_includes('!%^&*([{-=+|:;,?<>~') * lexer.range('/', true) * S('iomx')^0
local lit_regex = '%r' * literal_delimited * S('iomx')^0
local regex = token(lexer.REGEX, regex_str + lit_regex)
lex:add_rule('string', string + regex)

-- Numbers.
local dec = lexer.digit^1 * ('_' * lexer.digit^1)^0 * S('ri')^-1
local bin = '0b' * S('01')^1 * ('_' * S('01')^1)^0 * -lexer.xdigit
local integer = S('+-')^-1 * (bin + lexer.hex_num + lexer.oct_num + dec)
-- TODO: meta, control, etc. for numeric_literal.
local numeric_literal = '?' * (lexer.any - lexer.space) * -word_char
lex:add_rule('number', token(lexer.NUMBER, lexer.float * S('ri')^-1 + integer + numeric_literal))

-- Variables.
local global_var = '$' *
  (word + S('!@L+`\'=~/\\,.;<>_*"$?:') + lexer.digit + '-' * S('0FadiIKlpvw'))
local class_var = '@@' * word
local inst_var = '@' * word
lex:add_rule('variable', token(lexer.VARIABLE, global_var + class_var + inst_var))

-- Symbols.
lex:add_rule('symbol', token('symbol', ':' * P(function(input, index)
  if input:sub(index - 2, index - 2) ~= ':' then return index end
end) * (word_char^1 + sq_str + dq_str)))
lex:add_style('symbol', lexer.styles.constant)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('!%^&*()[]{}-=+/|:;.,?<>~')))

-- Fold points.
local function disambiguate(text, pos, line, s)
  return line:sub(1, s - 1):match('^%s*$') and not text:sub(1, pos - 1):match('\\[ \t]*\r?\n$') and
    1 or 0
end
lex:add_fold_point(lexer.KEYWORD, 'begin', 'end')
lex:add_fold_point(lexer.KEYWORD, 'class', 'end')
lex:add_fold_point(lexer.KEYWORD, 'def', 'end')
lex:add_fold_point(lexer.KEYWORD, 'do', 'end')
lex:add_fold_point(lexer.KEYWORD, 'for', 'end')
lex:add_fold_point(lexer.KEYWORD, 'module', 'end')
lex:add_fold_point(lexer.KEYWORD, 'case', 'end')
lex:add_fold_point(lexer.KEYWORD, 'if', disambiguate)
lex:add_fold_point(lexer.KEYWORD, 'while', disambiguate)
lex:add_fold_point(lexer.KEYWORD, 'unless', disambiguate)
lex:add_fold_point(lexer.KEYWORD, 'until', disambiguate)
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '=begin', '=end')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
