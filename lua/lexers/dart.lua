-- Dart LPeg lexer.
-- Written by Brian Schott (@Hackerpilot on Github).

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'dart'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline_esc^0
local nested_comment = l.nested_pair('/*', '*/')
local comment = token(l.COMMENT, line_comment + nested_comment)

-- Strings.
local sq_str = S('r')^-1 * l.delimited_range("'", true)
local dq_str = S('r')^-1 * l.delimited_range('"', true)
local sq_str_multiline = S('r')^-1 * l.delimited_range('"""')
local dq_str_multiline = S('r')^-1 * l.delimited_range("''' ")
local string = token(l.STRING,
                     sq_str + dq_str + sq_str_multiline + dq_str_multiline)

-- Numbers.
local number = token(l.NUMBER, (l.float + l.hex_num))

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'assert', 'break', 'case', 'catch', 'class', 'const', 'continue', 'default',
  'do', 'else', 'enum', 'extends', 'false', 'final' , 'finally', 'for', 'if',
  'in', 'is', 'new', 'null', 'rethrow', 'return', 'super', 'switch', 'this',
  'throw', 'true', 'try', 'var', 'void', 'while', 'with',
})

local builtin_identifiers = token(l.CONSTANT, word_match{
  'abstract', 'as', 'dynamic', 'export', 'external', 'factory', 'get',
  'implements', 'import', 'library', 'operator', 'part', 'set', 'static',
  'typedef'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('#?=!<>+-*$/%&|^~.,;()[]{}'))

-- Preprocs.
local annotation = token('annotation', '@' * l.word^1)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'constant', builtin_identifiers},
  {'string', string},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
  {'annotation', annotation},
}

M._tokenstyles = {
  annotation = l.STYLE_PREPROCESSOR,
}

M._foldsymbols = {
  _patterns = {'[{}]', '/[*+]', '[*+]/', '//'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {
    ['/*'] = 1, ['*/'] = -1, ['/+'] = 1, ['+/'] = -1,
    ['//'] = l.fold_line_comments('//')
  }
}

return M
