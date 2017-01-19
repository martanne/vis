-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- YAML LPeg lexer.
-- It does not keep track of indentation perfectly.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'yaml'}

-- Whitespace.
local indent = #l.starts_line(S(' \t')) *
               (token(l.WHITESPACE, ' ') + token('indent_error', '\t'))^1
local ws = token(l.WHITESPACE, S(' \t')^1 + l.newline^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local string = token(l.STRING, l.delimited_range("'") + l.delimited_range('"'))

-- Numbers.
local integer = l.dec_num + l.hex_num + '0' * S('oO') * R('07')^1
local special_num = '.' * word_match({'inf', 'nan'}, nil, true)
local number = token(l.NUMBER, special_num + l.float + integer)

-- Timestamps.
local ts = token('timestamp', l.digit * l.digit * l.digit * l.digit * -- year
                              '-' * l.digit * l.digit^-1 * -- month
                              '-' * l.digit * l.digit^-1 * -- day
                              ((S(' \t')^1 + S('tT'))^-1 * -- separator
                               l.digit * l.digit^-1 * -- hour
                               ':' * l.digit * l.digit * -- minute
                               ':' * l.digit * l.digit * -- second
                               ('.' * l.digit^0)^-1 * -- fraction
                               ('Z' + -- timezone
                                S(' \t')^0 * S('-+') * l.digit * l.digit^-1 *
                                (':' * l.digit * l.digit)^-1)^-1)^-1)

-- Constants.
local constant = token(l.CONSTANT,
                       word_match({'null', 'true', 'false'}, nil, true))

-- Types.
local type = token(l.TYPE, '!!' * word_match({
  -- Collection types.
  'map', 'omap', 'pairs', 'set', 'seq',
  -- Scalar types.
  'binary', 'bool', 'float', 'int', 'merge', 'null', 'str', 'timestamp',
  'value', 'yaml'
}, nil, true) + '!' * l.delimited_range('<>'))

-- Document boundaries.
local doc_bounds = token('document', l.starts_line(P('---') + '...'))

-- Directives
local directive = token('directive', l.starts_line('%') * l.nonnewline^1)

local word = (l.alpha + '-' * -l.space) * (l.alnum + '-')^0

-- Keys and literals.
local colon = S(' \t')^0 * ':' * (l.space + -1)
local key = token(l.KEYWORD,
                  #word * (l.nonnewline - colon)^1 * #colon *
                  P(function(input, index)
                    local line = input:sub(1, index - 1):match('[^\r\n]+$')
                    return not line:find('[%w-]+:') and index
                  end))
local value = #word * (l.nonnewline - l.space^0 * S(',]}'))^1
local block = S('|>') * S('+-')^-1 * (l.newline + -1) * function(input, index)
  local rest = input:sub(index)
  local level = #rest:match('^( *)')
  for pos, indent, line in rest:gmatch('() *()([^\r\n]+)') do
    if indent - pos < level and line ~= ' ' or level == 0 and pos > 1 then
      return index + pos - 1
    end
  end
  return #input + 1
end
local literal = token('literal', value + block)

-- Indicators.
local anchor = token(l.LABEL, '&' * word)
local alias = token(l.VARIABLE, '*' * word)
local tag = token('tag', '!' * word * P('!')^-1)
local reserved = token(l.ERROR, S('@`') * word)
local indicator_chars = token(l.OPERATOR, S('-?:,[]{}!'))

M._rules = {
  {'indent', indent},
  {'whitespace', ws},
  {'comment', comment},
  {'doc_bounds', doc_bounds},
  {'key', key},
  {'literal', literal},
  {'timestamp', ts},
  {'number', number},
  {'constant', constant},
  {'type', type},
  {'indicator', tag + indicator_chars + alias + anchor + reserved},
  {'directive', directive},
}

M._tokenstyles = {
  indent_error = 'back:red',
  document = l.STYLE_CONSTANT,
  literal = l.STYLE_DEFAULT,
  timestamp = l.STYLE_NUMBER,
  tag = l.STYLE_CLASS,
  directive = l.STYLE_PREPROCESSOR,
}

M._FOLDBYINDENTATION = true

return M
