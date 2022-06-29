-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Shell LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('bash')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'if', 'then', 'elif', 'else', 'fi', 'case', 'in', 'esac', 'while', 'for', 'do', 'done',
  'continue', 'local', 'return', 'select',
  -- Operators.
  '-a', '-b', '-c', '-d', '-e', '-f', '-g', '-h', '-k', '-p', '-r', '-s', '-t', '-u', '-w', '-x',
  '-O', '-G', '-L', '-S', '-N', '-nt', '-ot', '-ef', '-o', '-z', '-n', '-eq', '-ne', '-lt', '-le',
  '-gt', '-ge'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"')
local ex_str = lexer.range('`')
local heredoc = '<<' * P(function(input, index)
  local _, e, _, delimiter = input:find('^%-?(["\']?)([%a_][%w_]*)%1[\n\r\f;]+', index)
  if not delimiter then return end
  _, e = input:find('[\n\r\f]+' .. delimiter, e)
  return e and e + 1 or #input + 1
end)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + ex_str + heredoc))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, '$' *
  (S('!#?*@$') + lexer.digit^1 + lexer.word + lexer.range('{', '}', true, false, true))))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=!<>+-/*^&|~.,:;?()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'if', 'fi')
lex:add_fold_point(lexer.KEYWORD, 'case', 'esac')
lex:add_fold_point(lexer.KEYWORD, 'do', 'done')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
