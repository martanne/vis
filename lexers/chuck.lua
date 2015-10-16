--------------------------------------------------------------------------------
-- The MIT License
--
-- Copyright (c) 2010 Martin Morawetz
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
-- THE SOFTWARE.
--------------------------------------------------------------------------------

-- Based on lexer code from Mitchell mitchell.att.foicica.com.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'chuck'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = P('L')^-1 * l.delimited_range("'", true)
local dq_str = P('L')^-1 * l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Constants.
local constant = token(l.CONSTANT, word_match{
  -- special values
  'false', 'maybe', 'me', 'null', 'NULL', 'pi', 'true'
})

-- Special special value.
local now = token('now', P('now'))

-- Times.
local time = token('time', word_match{
  'samp', 'ms', 'second', 'minute', 'hour', 'day', 'week'
})

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  -- Control structures.
  'break', 'continue', 'else', 'for', 'if', 'repeat', 'return', 'switch',
  'until', 'while',
  -- Other chuck keywords.
  'function', 'fun', 'spork', 'const', 'new'
})

-- Classes.
local class = token(l.CLASS, word_match{
  -- Class keywords.
  'class', 'extends', 'implements', 'interface', 'private', 'protected',
  'public', 'pure', 'super', 'static', 'this'
})

-- Types.
local types = token(l.TYPE, word_match{
  'float', 'int', 'time', 'dur', 'void', 'same'
})

-- Global ugens.
local ugen = token('ugen', word_match{'dac', 'adc', 'blackhole'})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}@'))

M._rules = {
  {'whitespace', ws},
  {'string', string},
  {'keyword', keyword},
  {'constant', constant},
  {'type', types},
  {'class', class},
  {'ugen', ugen},
  {'time', time},
  {'now', now},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._tokenstyles = {
  ugen = l.STYLE_CONSTANT,
  time = l.STYLE_NUMBER,
  now = l.STYLE_CONSTANT..',bold'
}

return M
