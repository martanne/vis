-- Copyright 2017-2022 Murray Calavera. See LICENSE.
-- Pony LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('pony')

-- Whitespace.
local ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', ws)

-- Capabilities.
local capability = token(lexer.LABEL, word_match('box iso ref tag trn val'))
lex:add_rule('capability', capability)

-- Annotations.
local annotation = token(lexer.PREPROCESSOR, lexer.range('\\', false, false))
lex:add_rule('annotation', annotation)

-- Functions.
-- Highlight functions with syntax sugar at declaration.
lex:add_rule('function',
  token(lexer.KEYWORD, word_match('fun new be')) * ws^-1 * annotation^-1 * ws^-1 * capability^-1 *
    ws^-1 * token(lexer.FUNCTION, word_match{
    'create', 'dispose', '_final', 'apply', 'update', 'add', 'sub', 'mul', 'div', 'mod',
    'add_unsafe', 'sub_unsafe', 'mul_unsafe', 'div_unsafe', 'mod_unsafe', 'shl', 'shr',
    'shl_unsafe', 'shr_unsafe', 'op_and', 'op_or', 'op_xor', 'eq', 'ne', 'lt', 'le', 'ge', 'gt',
    'eq_unsafe', 'ne_unsafe', 'lt_unsafe', 'le_unsafe', 'ge_unsafe', 'gt_unsafe', 'neg',
    'neg_unsafe', 'op_not', --
    'has_next', 'next', --
    '_serialise_space', '_serialise', '_deserialise'
  }))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'actor', 'as', 'be', 'break', 'class', 'compile_error', 'compile_intrinsic', 'continue',
  'consume', 'do', 'else', 'elseif', 'embed', 'end', 'error', 'for', 'fun', 'if', 'ifdef', 'iftype',
  'in', 'interface', 'is', 'isnt', 'lambda', 'let', 'match', 'new', 'object', 'primitive',
  'recover', 'repeat', 'return', 'struct', 'then', 'this', 'trait', 'try', 'type', 'until', 'use',
  'var', 'where', 'while', 'with'
}))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match('true false')))

-- Operators.
local ops = {
  ['+'] = true, ['-'] = true, ['*'] = true, ['/'] = true, ['%'] = true, ['+~'] = true,
  ['-~'] = true, ['*~'] = true, ['/~'] = true, ['%~'] = true, ['<<'] = true, ['>>'] = true,
  ['<<~'] = true, ['>>~'] = true, ['=='] = true, ['!='] = true, ['<'] = true, ['<='] = true,
  ['>='] = true, ['>'] = true, ['==~'] = true, ['!=~'] = true, ['<~'] = true, ['<=~'] = true,
  ['>=~'] = true, ['>~'] = true
}
lex:add_rule('operator', token(lexer.OPERATOR, word_match('and or xor not addressof digestof') +
  lpeg.Cmt(S('+-*/%<>=!~')^1, function(input, index, op) return ops[op] and index or nil end)))

-- Identifiers.
local id_suffix = (lexer.alnum + "'" + '_')^0
lex:add_rule('type', token(lexer.TYPE, P('_')^-1 * lexer.upper * id_suffix))
lex:add_rule('identifier', token(lexer.IDENTIFIER, P('_')^-1 * lexer.lower * id_suffix))
lex:add_rule('lookup', token(lexer.IDENTIFIER, '_' * lexer.digit^1))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local tq_str = lexer.range('"""')
lex:add_rule('string', token(lexer.STRING, sq_str + tq_str + dq_str))

-- Numbers.
local function num(digit) return digit * (digit^0 * '_')^0 * digit^1 + digit end
local int = num(lexer.digit)
local frac = '.' * int
local exp = S('eE') * (P('-') + '+')^-1 * int
local hex = '0x' * num(lexer.xdigit)
local bin = '0b' * num(S('01'))
local float = int * frac^-1 * exp^-1
lex:add_rule('number', token(lexer.NUMBER, hex + bin + float))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/', false, false, true)
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Punctuation.
-- There is no suitable token name for this, change this if ever one is added.
lex:add_rule('punctuation',
  token(lexer.OPERATOR, P('=>') + '.>' + '<:' + '->' + S('=.,:;()[]{}!?~^&|_@')))

-- Qualifiers.
lex:add_rule('qualifier', token(lexer.LABEL, '#' * word_match('read send share any alias')))

return lex
