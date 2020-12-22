-- Copyright 2006-2020 Mitchell. See LICENSE.
-- YAML LPeg lexer.
-- It does not keep track of indentation perfectly.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local M = {_NAME = 'yaml'}

-- Whitespace.
local indent = #lexer.starts_line(S(' \t')) *
  (token(lexer.WHITESPACE, ' ') + token('indent_error', '\t'))^1
local ws = token(lexer.WHITESPACE, S(' \t')^1 + lexer.newline^1)

-- Comments.
local comment = token(lexer.COMMENT, lexer.to_eol('#'))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local string = token(lexer.STRING, sq_str + dq_str)

-- Numbers.
local integer = lexer.dec_num + lexer.hex_num + '0' * S('oO') * lpeg.R('07')^1
local special_num = '.' * word_match({'inf', 'nan'}, nil, true)
local number = token(lexer.NUMBER, special_num + lexer.float + integer)

-- Timestamps.
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
local ts = token('timestamp', date * (T * time * zone^-1))

-- Constants.
local constant = token(lexer.CONSTANT, word_match({
  'null', 'true', 'false'
}, nil, true))

-- Types.
local type = token(lexer.TYPE, '!!' * word_match({
  -- Collection types.
  'map', 'omap', 'pairs', 'set', 'seq',
  -- Scalar types.
  'binary', 'bool', 'float', 'int', 'merge', 'null', 'str', 'timestamp',
  'value', 'yaml'
}, nil, true) + '!' * lexer.range('<', '>', true))

-- Document boundaries.
local doc_bounds = token('document', lexer.starts_line(P('---') + '...'))

-- Directives
local directive = token('directive', lexer.starts_line('%') *
  lexer.nonnewline^1)

local word = (lexer.alpha + '-' * -lexer.space) * (lexer.alnum + '-')^0

-- Keys and literals.
local colon = S(' \t')^0 * ':' * (lexer.space + -1)
local key = token(lexer.KEYWORD, #word * (lexer.nonnewline - colon)^1 * #colon *
  P(function(input, index)
    local line = input:sub(1, index - 1):match('[^\r\n]+$')
    return not line:find('[%w-]+:') and index
  end))
local value = #word * (lexer.nonnewline - lexer.space^0 * S(',]}'))^1
local block = S('|>') * S('+-')^-1 * (lexer.newline + -1) *
  function(input, index)
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
local anchor = token(lexer.LABEL, '&' * word)
local alias = token(lexer.VARIABLE, '*' * word)
local tag = token('tag', '!' * word * P('!')^-1)
local reserved = token(lexer.ERROR, S('@`') * word)
local indicator_chars = token(lexer.OPERATOR, S('-?:,[]{}!'))

M._rules = {
  {'indent', indent},
  {'whitespace', ws},
  {'comment', comment},
  {'doc_bounds', doc_bounds},
  {'key', key},
  {'string', string},
  {'literal', literal},
  {'timestamp', ts},
  {'number', number},
  {'constant', constant},
  {'type', type},
  {'indicator', tag + indicator_chars + alias + anchor + reserved},
  {'directive', directive},
}

M._tokenstyles = {
  indent_error = 'back:%(color.red)',
  document = lexer.STYLE_CONSTANT,
  literal = lexer.STYLE_DEFAULT,
  timestamp = lexer.STYLE_NUMBER,
  tag = lexer.STYLE_CLASS,
  directive = lexer.STYLE_PREPROCESSOR,
}

M._FOLDBYINDENTATION = true

return M
