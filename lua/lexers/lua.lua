-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Lua LPeg lexer.
-- Original written by Peter Odding, 2007/04/04.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'lua'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

local longstring = lpeg.Cmt('[' * lpeg.C(P('=')^0) * '[',
                            function(input, index, eq)
                              local _, e = input:find(']'..eq..']', index, true)
                              return (e or #input) + 1
                            end)

-- Comments.
local line_comment = '--' * l.nonnewline^0
local block_comment = '--' * longstring
local comment = token(l.COMMENT, block_comment + line_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local string = token(l.STRING, sq_str + dq_str) +
               token('longstring', longstring)

-- Numbers.
local lua_integer = P('-')^-1 * (l.hex_num + l.dec_num)
local number = token(l.NUMBER, l.float + lua_integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', 'function',
  'goto', 'if', 'in', 'local', 'nil', 'not', 'or', 'repeat', 'return', 'then',
  'true', 'until', 'while'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'assert', 'collectgarbage', 'dofile', 'error', 'getmetatable', 'ipairs',
  'load', 'loadfile', 'next', 'pairs', 'pcall', 'print', 'rawequal', 'rawget',
  'rawset', 'require', 'select', 'setmetatable', 'tonumber', 'tostring', 'type',
  'xpcall',
  -- Added in 5.2.
  'rawlen'
})

-- Deprecated functions.
local deprecated_func = token('deprecated_function', word_match{
  -- Deprecated in 5.2.
  'getfenv', 'loadstring', 'module', 'setfenv', 'unpack'
})

-- Constants.
local constant = token(l.CONSTANT, word_match{
  '_G', '_VERSION',
  -- Added in 5.2.
  '_ENV'
})

-- Libraries.
local library = token('library', word_match({
  -- Coroutine.
  'coroutine', 'coroutine.create', 'coroutine.resume', 'coroutine.running',
  'coroutine.status', 'coroutine.wrap', 'coroutine.yield',
  -- Coroutine added in 5.3.
  'coroutine.isyieldable',
  -- Module.
  'package', 'package.cpath', 'package.loaded', 'package.loadlib',
  'package.path', 'package.preload',
  -- Module added in 5.2.
  'package.config', 'package.searchers', 'package.searchpath',
  -- UTF-8 added in 5.3.
  'utf8', 'utf8.char', 'utf8.charpattern', 'utf8.codepoint', 'utf8.codes',
  'utf8.len', 'utf8.offset',
  -- String.
  'string', 'string.byte', 'string.char', 'string.dump', 'string.find',
  'string.format', 'string.gmatch', 'string.gsub', 'string.len', 'string.lower',
  'string.match', 'string.rep', 'string.reverse', 'string.sub', 'string.upper',
  -- String added in 5.3.
  'string.pack', 'string.packsize', 'string.unpack',
  -- Table.
  'table', 'table.concat', 'table.insert', 'table.remove', 'table.sort',
  -- Table added in 5.2.
  'table.pack', 'table.unpack',
  -- Table added in 5.3.
  'table.move',
  -- Math.
  'math', 'math.abs', 'math.acos', 'math.asin', 'math.atan', 'math.ceil',
  'math.cos', 'math.deg', 'math.exp', 'math.floor', 'math.fmod', 'math.huge',
  'math.log', 'math.max', 'math.min', 'math.modf', 'math.pi', 'math.rad',
  'math.random', 'math.randomseed', 'math.sin', 'math.sqrt', 'math.tan',
  -- Math added in 5.3.
  'math.maxinteger', 'math.mininteger', 'math.tointeger', 'math.type',
  'math.ult',
  -- IO.
  'io', 'io.close', 'io.flush', 'io.input', 'io.lines', 'io.open', 'io.output',
  'io.popen', 'io.read', 'io.stderr', 'io.stdin', 'io.stdout', 'io.tmpfile',
  'io.type', 'io.write',
  -- OS.
  'os', 'os.clock', 'os.date', 'os.difftime', 'os.execute', 'os.exit',
  'os.getenv', 'os.remove', 'os.rename', 'os.setlocale', 'os.time',
  'os.tmpname',
  -- Debug.
  'debug', 'debug.debug', 'debug.gethook', 'debug.getinfo', 'debug.getlocal',
  'debug.getmetatable', 'debug.getregistry', 'debug.getupvalue',
  'debug.sethook', 'debug.setlocal', 'debug.setmetatable', 'debug.setupvalue',
  'debug.traceback',
  -- Debug added in 5.2.
  'debug.getuservalue', 'debug.setuservalue', 'debug.upvalueid',
  'debug.upvaluejoin',
}, '.'))

-- Deprecated libraries.
local deprecated_library = token('deprecated_library', word_match({
  -- Module deprecated in 5.2.
  'package.loaders', 'package.seeall',
  -- Table deprecated in 5.2.
  'table.maxn',
  -- Math deprecated in 5.2.
  'math.log10',
  -- Math deprecated in 5.3.
  'math.atan2', 'math.cosh', 'math.frexp', 'math.ldexp', 'math.pow',
  'math.sinh', 'math.tanh',
  -- Bit32 deprecated in 5.3.
  'bit32', 'bit32.arshift', 'bit32.band', 'bit32.bnot', 'bit32.bor',
  'bit32.btest', 'bit32.extract', 'bit32.lrotate', 'bit32.lshift',
  'bit32.replace', 'bit32.rrotate', 'bit32.rshift', 'bit32.xor',
  -- Debug deprecated in 5.2.
  'debug.getfenv', 'debug.setfenv'
}, '.'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Labels.
local label = token(l.LABEL, '::' * l.word * '::')

-- Operators.
local operator = token(l.OPERATOR, S('+-*/%^#=<>&|~;:,.{}[]()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func + deprecated_func},
  {'constant', constant},
  {'library', library + deprecated_library},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'label', label},
  {'operator', operator},
}

M._tokenstyles = {
  longstring = l.STYLE_STRING,
  deprecated_function = l.STYLE_FUNCTION..',italics',
  library = l.STYLE_TYPE,
  deprecated_library = l.STYLE_TYPE..',italics'
}

local function fold_longcomment(text, pos, line, s, match)
  if match == '[' then
    if line:find('^%[=*%[', s) then return 1 end
  elseif match == ']' then
    if line:find('^%]=*%]', s) then return -1 end
  end
  return 0
end

M._foldsymbols = {
  _patterns = {'%l+', '[%({%)}]', '[%[%]]', '%-%-'},
  [l.KEYWORD] = {
    ['if'] = 1, ['do'] = 1, ['function'] = 1, ['end'] = -1, ['repeat'] = 1,
    ['until'] = -1
  },
  [l.COMMENT] = {
    ['['] = fold_longcomment, [']'] = fold_longcomment,
    ['--'] = l.fold_line_comments('--')
  },
  longstring = {['['] = 1, [']'] = -1},
  [l.OPERATOR] = {['('] = 1, ['{'] = 1, [')'] = -1, ['}'] = -1}
}

return M
