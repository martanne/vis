-- Copyright 2015-2017 Jason Schindler. See LICENSE.
-- Fish (http://fishshell.com/) script LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'fish'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- shebang
local shebang = token('shebang', '#!/' * l.nonnewline^0)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"')

local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'alias', 'and', 'begin', 'bg', 'bind', 'block', 'break', 'breakpoint',
  'builtin', 'case', 'cd', 'command', 'commandline', 'complete', 'contains',
  'continue', 'count', 'dirh', 'dirs', 'echo', 'else', 'emit', 'end', 'eval',
  'exec', 'exit', 'fg', 'fish', 'fish_config', 'fish_indent', 'fish_pager',
  'fish_prompt', 'fish_right_prompt', 'fish_update_completions', 'fishd', 'for',
  'funced', 'funcsave', 'function', 'functions', 'help', 'history', 'if', 'in',
  'isatty', 'jobs', 'math', 'mimedb', 'nextd', 'not', 'open', 'or', 'popd',
  'prevd', 'psub', 'pushd', 'pwd', 'random', 'read', 'return', 'set',
  'set_color', 'source', 'status', 'switch', 'test', 'trap', 'type', 'ulimit',
  'umask', 'vared', 'while'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local variable = token(l.VARIABLE,
                       '$' * l.word + '$' * l.delimited_range('{}', true, true))

-- Operators.
local operator = token(l.OPERATOR, S('=!<>+-/*^&|~.,:;?()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'shebang', shebang},
  {'keyword', keyword},
  {'identifier', identifier},
  {'variable', variable},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._tokenstyles = {
  shebang = l.STYLE_LABEL
}

M._foldsymbols = {
  _patterns = {'%l+'},
  [l.KEYWORD] = {
    begin = 1, ['for'] = 1, ['function'] = 1, ['if'] = 1, switch = 1,
    ['while'] = 1, ['end'] = -1
  }
}

return M
