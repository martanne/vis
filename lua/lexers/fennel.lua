-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Lua LPeg lexer.
-- Original written by Peter Odding, 2007/04/04.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'fennel'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = ';' * l.nonnewline^0
local comment = token(l.COMMENT, line_comment)

-- Strings.
local dq_str = l.delimited_range('"')
local string = token(l.STRING, dq_str)

-- Numbers.
local lua_integer = P('-')^-1 * (l.hex_num + l.dec_num)
local number = token(l.NUMBER, l.float + lua_integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  '%', '*', '+', '-', '->', '->>', '-?>', '-?>>', '.', '..', '/', '//', ':', '<', '<=', '=', '>', '>=', '^', '~=', 'λ',
  'and', 'comment', 'do', 'doc', 'doto', 'each', 'eval-compiler', 'fn', 'for', 'global', 'hashfn', 'if', 'include', 'lambda',
  'length', 'let', 'local', 'lua', 'macro', 'macros', 'match', 'not', 'not=', 'or', 'partial', 'quote', 'require-macros',
  'set', 'set-forcibly!', 'tset', 'values', 'var', 'when', 'while'
}, "%*+-./:<=>?~^λ!"))

-- Libraries.
local library = token('library', word_match({
  -- Coroutine.
  'coroutine', 'coroutine.create', 'coroutine.resume', 'coroutine.running',
  'coroutine.status', 'coroutine.wrap', 'coroutine.yield',
  -- Module.
  'package', 'package.cpath', 'package.loaded', 'package.loadlib',
  'package.path', 'package.preload',
  -- String.
  'string', 'string.byte', 'string.char', 'string.dump', 'string.find',
  'string.format', 'string.gmatch', 'string.gsub', 'string.len', 'string.lower',
  'string.match', 'string.rep', 'string.reverse', 'string.sub', 'string.upper',
  -- Table.
  'table', 'table.concat', 'table.insert', 'table.remove', 'table.sort',
  -- Math.
  'math', 'math.abs', 'math.acos', 'math.asin', 'math.atan', 'math.ceil',
  'math.cos', 'math.deg', 'math.exp', 'math.floor', 'math.fmod', 'math.huge',
  'math.log', 'math.max', 'math.min', 'math.modf', 'math.pi', 'math.rad',
  'math.random', 'math.randomseed', 'math.sin', 'math.sqrt', 'math.tan',
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
}, '.'))

local initial = l.alpha + S"|$%&#*+-./:<=>?~^_λ!"
local subsequent = initial + l.digit

-- Identifiers.
local identifier = token(l.IDENTIFIER, initial * subsequent^0)

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'library', library},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number}
}

M._tokenstyles = {
  library = l.STYLE_TYPE,
}

return M
