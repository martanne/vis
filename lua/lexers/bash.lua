-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Shell LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'bash'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"')
local ex_str = l.delimited_range('`')
local heredoc = '<<' * P(function(input, index)
  local s, e, _, delimiter =
    input:find('%-?(["\']?)([%a_][%w_]*)%1[\n\r\f;]+', index)
  if s == index and delimiter then
    local _, e = input:find('[\n\r\f]+'..delimiter, e)
    return e and e + 1 or #input + 1
  end
end)
local string = token(l.STRING, sq_str + dq_str + ex_str + heredoc)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'if', 'then', 'elif', 'else', 'fi', 'case', 'in', 'esac', 'while', 'for',
  'do', 'done', 'continue', 'local', 'return', 'select',
  -- Operators.
  '-a', '-b', '-c', '-d', '-e', '-f', '-g', '-h', '-k', '-p', '-r', '-s', '-t',
  '-u', '-w', '-x', '-O', '-G', '-L', '-S', '-N', '-nt', '-ot', '-ef', '-o',
  '-z', '-n', '-eq', '-ne', '-lt', '-le', '-gt', '-ge'
}, '-'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local variable = token(l.VARIABLE,
                       '$' * (S('!#?*@$') + l.digit^1 + l.word +
                              l.delimited_range('{}', true, true, true)))

-- Operators.
local operator = token(l.OPERATOR, S('=!<>+-/*^&|~.,:;?()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'variable', variable},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '[{}]', '#'},
  [l.KEYWORD] = {
    ['if'] = 1, fi = -1, case = 1, esac = -1, ['do'] = 1, done = -1
  },
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
