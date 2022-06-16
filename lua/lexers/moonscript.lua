-- Copyright 2016-2022 Alejandro Baez (https://keybase.io/baez). See LICENSE.
-- Moonscript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('moonscript', {fold_by_indentation = true})

-- Whitespace.
lex:add_rule('whitspace', token(lexer.WHITESPACE, lexer.space^1))

-- Table keys.
lex:add_rule('tbl_key', token('tbl_key', lexer.word * ':' + ':' * lexer.word))
lex:add_style('tbl_key', lexer.STYLE_REGEX)

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- Lua.
  'and', 'break', 'do', 'else', 'elseif', 'false', 'for', 'if', 'in', 'local', 'nil', 'not', 'or',
  'return', 'then', 'true', 'while',
  -- Moonscript.
  'continue', 'class', 'export', 'extends', 'from', 'import', 'super', 'switch', 'unless', 'using',
  'when', 'with'
}))

-- Error words.
lex:add_rule('error', token(lexer.ERROR, word_match('function end')))

-- Self reference.
lex:add_rule('self_ref', token('self_ref', '@' * lexer.word^-1 + 'self'))
lex:add_style('self_ref', lexer.styles.label)

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'assert', 'collectgarbage', 'dofile', 'error', 'getmetatable', 'ipairs', 'load', 'loadfile',
  'next', 'pairs', 'pcall', 'print', 'rawequal', 'rawget', 'rawset', 'require', 'select',
  'setmetatable', 'tonumber', 'tostring', 'type', 'xpcall',
  -- Added in 5.2.
  'rawlen'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
  '_G', '_VERSION',
  -- Added in 5.2.
  '_ENV'
}))

-- Libraries.
lex:add_rule('library', token('library', word_match{
  -- Coroutine.
  'coroutine', 'coroutine.create', 'coroutine.resume', 'coroutine.running', 'coroutine.status',
  'coroutine.wrap', 'coroutine.yield',
  -- Coroutine added in 5.3.
  'coroutine.isyieldable',
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
  'debug.getuservalue', 'debug.setuservalue', 'debug.upvalueid', 'debug.upvaluejoin',

  --- MoonScript 0.3.1 standard library.
  -- Printing functions.
  'p',
  -- Table functions.
  'run_with_scope', 'defaultbl', 'extend', 'copy',
  -- Class/object functions.
  'is_object', 'bind_methods', 'mixin', 'mixin_object', 'mixin_table',
  -- Misc functions.
  'fold',
  -- Debug functions.
  'debug.upvalue'
}))
lex:add_style('library', lexer.styles.type)

-- Identifiers.
local identifier = token(lexer.IDENTIFIER, lexer.word)
local proper_ident = token('proper_ident', lexer.upper * lexer.word)
lex:add_rule('identifier', proper_ident + identifier)
lex:add_style('proper_ident', lexer.styles.class)

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"', false, false)
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
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Function definition.
lex:add_rule('fndef', token('fndef', P('->') + '=>'))
lex:add_style('fndef', lexer.styles.preprocessor)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-*!\\/%^#=<>;:,.')))
lex:add_rule('symbol', token('symbol', S('(){}[]')))
lex:add_style('symbol', lexer.styles.embedded)

return lex
