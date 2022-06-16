-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Lua LPeg lexer.
-- Original written by Peter Odding, 2007/04/04.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local B, P, S = lpeg.B, lpeg.P, lpeg.S

local lex = lexer.new('lua')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'and', 'break', 'do', 'else', 'elseif', 'end', 'false', 'for', 'function', 'if', 'in', 'local',
  'nil', 'not', 'or', 'repeat', 'return', 'then', 'true', 'until', 'while',
  -- Added in 5.2.
  'goto'
}))

-- Functions and deprecated functions.
local func = token(lexer.FUNCTION, word_match{
  'assert', 'collectgarbage', 'dofile', 'error', 'getmetatable', 'ipairs', 'load', 'loadfile',
  'next', 'pairs', 'pcall', 'print', 'rawequal', 'rawget', 'rawset', 'require', 'select',
  'setmetatable', 'tonumber', 'tostring', 'type', 'xpcall',
  -- Added in 5.2.
  'rawlen',
  -- Added in 5.4.
  'warn'
})
local deprecated_func = token('deprecated_function', word_match{
  -- Deprecated in 5.2.
  'getfenv', 'loadstring', 'module', 'setfenv', 'unpack'
})
lex:add_rule('function', -B('.') * (func + deprecated_func))
lex:add_style('deprecated_function', lexer.styles['function'] .. {italics = true})

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, -B('.') * word_match{
  '_G', '_VERSION',
  -- Added in 5.2.
  '_ENV'
}))

-- Libraries and deprecated libraries.
local library = token('library', word_match{
  -- Coroutine.
  'coroutine', 'coroutine.create', 'coroutine.resume', 'coroutine.running', 'coroutine.status',
  'coroutine.wrap', 'coroutine.yield',
  -- Coroutine added in 5.3.
  'coroutine.isyieldable',
  -- Coroutine added in 5.4.
  'coroutine.close',
  -- Module.
  'package', 'package.cpath', 'package.loaded', 'package.loadlib', 'package.path',
  'package.preload',
  -- Module added in 5.2.
  'package.config', 'package.searchers', 'package.searchpath',
  -- UTF-8 added in 5.3.
  'utf8', 'utf8.char', 'utf8.charpattern', 'utf8.codepoint', 'utf8.codes', 'utf8.len',
  'utf8.offset',
  -- String.
  'string', 'string.byte', 'string.char', 'string.dump', 'string.find', 'string.format',
  'string.gmatch', 'string.gsub', 'string.len', 'string.lower', 'string.match', 'string.rep',
  'string.reverse', 'string.sub', 'string.upper',
  -- String added in 5.3.
  'string.pack', 'string.packsize', 'string.unpack',
  -- Table.
  'table', 'table.concat', 'table.insert', 'table.remove', 'table.sort',
  -- Table added in 5.2.
  'table.pack', 'table.unpack',
  -- Table added in 5.3.
  'table.move',
  -- Math.
  'math', 'math.abs', 'math.acos', 'math.asin', 'math.atan', 'math.ceil', 'math.cos', 'math.deg',
  'math.exp', 'math.floor', 'math.fmod', 'math.huge', 'math.log', 'math.max', 'math.min',
  'math.modf', 'math.pi', 'math.rad', 'math.random', 'math.randomseed', 'math.sin', 'math.sqrt',
  'math.tan',
  -- Math added in 5.3.
  'math.maxinteger', 'math.mininteger', 'math.tointeger', 'math.type', 'math.ult',
  -- IO.
  'io', 'io.close', 'io.flush', 'io.input', 'io.lines', 'io.open', 'io.output', 'io.popen',
  'io.read', 'io.stderr', 'io.stdin', 'io.stdout', 'io.tmpfile', 'io.type', 'io.write',
  -- OS.
  'os', 'os.clock', 'os.date', 'os.difftime', 'os.execute', 'os.exit', 'os.getenv', 'os.remove',
  'os.rename', 'os.setlocale', 'os.time', 'os.tmpname',
  -- Debug.
  'debug', 'debug.debug', 'debug.gethook', 'debug.getinfo', 'debug.getlocal', 'debug.getmetatable',
  'debug.getregistry', 'debug.getupvalue', 'debug.sethook', 'debug.setlocal', 'debug.setmetatable',
  'debug.setupvalue', 'debug.traceback',
  -- Debug added in 5.2.
  'debug.getuservalue', 'debug.setuservalue', 'debug.upvalueid', 'debug.upvaluejoin'
})
local deprecated_library = token('deprecated_library', word_match{
  -- Module deprecated in 5.2.
  'package.loaders', 'package.seeall',
  -- Table deprecated in 5.2.
  'table.maxn',
  -- Math deprecated in 5.2.
  'math.log10',
  -- Math deprecated in 5.3.
  'math.atan2', 'math.cosh', 'math.frexp', 'math.ldexp', 'math.pow', 'math.sinh', 'math.tanh',
  -- Bit32 deprecated in 5.3.
  'bit32', 'bit32.arshift', 'bit32.band', 'bit32.bnot', 'bit32.bor', 'bit32.btest', 'bit32.extract',
  'bit32.lrotate', 'bit32.lshift', 'bit32.replace', 'bit32.rrotate', 'bit32.rshift', 'bit32.xor',
  -- Debug deprecated in 5.2.
  'debug.getfenv', 'debug.setfenv'
})
lex:add_rule('library', -B('.') * (library + deprecated_library))
lex:add_style('library', lexer.styles.type)
lex:add_style('deprecated_library', lexer.styles.type .. {italics = true})

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local longstring = lpeg.Cmt('[' * lpeg.C(P('=')^0) * '[', function(input, index, eq)
  local _, e = input:find(']' .. eq .. ']', index, true)
  return (e or #input) + 1
end)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str) + token('longstring', longstring))
lex:add_style('longstring', lexer.styles.string)

-- Comments.
local line_comment = lexer.to_eol('--')
local block_comment = '--' * longstring
lex:add_rule('comment', token(lexer.COMMENT, block_comment + line_comment))

-- Numbers.
local lua_integer = P('-')^-1 * (lexer.hex_num + lexer.dec_num)
lex:add_rule('number', token(lexer.NUMBER, lexer.float + lua_integer))

-- Labels.
lex:add_rule('label', token(lexer.LABEL, '::' * lexer.word * '::'))

-- Attributes.
lex:add_rule('attribute', token('attribute', '<' * lexer.space^0 * word_match('const close') *
  lexer.space^0 * '>'))
lex:add_style('attribute', lexer.styles.class)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, '..' + S('+-*/%^#=<>&|~;:,.{}[]()')))

-- Fold points.
local function fold_longcomment(text, pos, line, s, symbol)
  if symbol == '[' then
    if line:find('^%[=*%[', s) then return 1 end
  elseif symbol == ']' then
    if line:find('^%]=*%]', s) then return -1 end
  end
  return 0
end
lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
lex:add_fold_point(lexer.KEYWORD, 'do', 'end')
lex:add_fold_point(lexer.KEYWORD, 'function', 'end')
lex:add_fold_point(lexer.KEYWORD, 'repeat', 'until')
lex:add_fold_point(lexer.COMMENT, '[', fold_longcomment)
lex:add_fold_point(lexer.COMMENT, ']', fold_longcomment)
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('--'))
lex:add_fold_point('longstring', '[', ']')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')

return lex
