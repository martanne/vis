-- Copyright 2021-2022 Mitchell. See LICENSE.
-- Gleam LPeg lexer
-- https://gleam.run/
-- Contributed by Tynan Beatty

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local KEY, OP = lexer.KEYWORD, lexer.OPERATOR

local lex = lexer.new('gleam')

-- Whitespace.
local gleam_ws = token(lexer.WHITESPACE, lexer.space^1)
lex:add_rule('whitespace', gleam_ws)

-- Types.
local typ_tok = token(lexer.TYPE, lexer.upper * lexer.alnum^0)
lex:add_rule('type', typ_tok)

-- Modules.
local name = (lexer.lower + '_') * (lexer.lower + lexer.digit + '_')^0
local fn_name = token(lexer.FUNCTION, name)
local mod_name = token('module', name)
local typ_or_fn = typ_tok + fn_name
local function mod_tok(ws)
  return token(KEY, 'import') * ws^1 * mod_name * (ws^0 * token(OP, '/') * ws^0 * mod_name)^0 *
    (ws^1 * token(KEY, 'as') * ws^1 * mod_name)^-1 *
    (ws^0 * token(OP, '.') * ws^0 * token(OP, '{') * ws^0 * typ_or_fn *
      (ws^0 * token(OP, ',') * ws^0 * typ_or_fn)^0 * ws^0 * token(OP, '}'))^-1
end
lex:add_rule('module', mod_tok(gleam_ws))
lex:add_style('module', lexer.styles.constant)

-- Keywords.
local key_tok = token(KEY, word_match(
  'as assert case const external fn if import let opaque pub todo try tuple type'))
lex:add_rule('keyword', key_tok)

-- Functions.
local function fn_tok(ws)
  local mod_name_op = mod_name * ws^0 * token(OP, '.')
  local fn_def_call = mod_name_op^-1 * ws^0 * fn_name * ws^0 * #P('(')
  local fn_pipe = token(OP, '|>') * ws^0 * (token(KEY, 'fn') + mod_name_op^-1 * fn_name)
  return fn_def_call + fn_pipe
end
lex:add_rule('function', fn_tok(gleam_ws))

-- Labels.
local id = token(lexer.IDENTIFIER, name)
local function lab_tok(ws)
  return token(OP, S('(,')) * ws^0 * token(lexer.LABEL, name) * #(ws^1 * id)
end
lex:add_rule('label', lab_tok(gleam_ws))

-- Identifiers.
local discard_id = token('discard', '_' * name)
local id_tok = discard_id + id
lex:add_rule('identifier', id_tok)
lex:add_style('discard', lexer.styles.comment)

-- Strings.
local str_tok = token(lexer.STRING, lexer.range('"'))
lex:add_rule('string', str_tok)

-- Comments.
local com_tok = token(lexer.COMMENT, lexer.to_eol('//'))
lex:add_rule('comment', com_tok)

-- Numbers.
local function can_neg(patt) return (lpeg.B(lexer.space + S('+-/*%<>=&|:,.')) * '-')^-1 * patt end
local function can_sep(patt) return (P('_')^-1 * patt^1)^1 end
local dec = lexer.digit * can_sep(lexer.digit)^0
local float = dec * '.' * dec^0
local bin = '0' * S('bB') * can_sep(S('01')) * -lexer.xdigit
local oct = '0' * S('oO') * can_sep(lpeg.R('07'))
local hex = '0' * S('xX') * can_sep(lexer.xdigit)
local num_tok = token(lexer.NUMBER, can_neg(float) + bin + oct + hex + can_neg(dec))
lex:add_rule('number', num_tok)

-- Operators.
local op_tok = token(OP, S('+-*/%#!=<>&|.,:;{}[]()'))
lex:add_rule('operator', op_tok)

-- Errors.
local err_tok = token(lexer.ERROR, lexer.any)
lex:add_rule('error', err_tok)

-- Fold points.
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '(', ')')

-- Embedded Bit Strings.
-- Mimic lexer.load() by creating a bitstring-specific whitespace style.
local bitstring = lexer.new(lex._NAME .. '_bitstring')
local bitstring_ws = token(bitstring._NAME .. '_whitespace', lexer.space^1)
bitstring:add_rule('whitespace', bitstring_ws)
bitstring:add_style(bitstring._NAME .. '_whitespace', lexer.styles.whitespace)
bitstring:add_rule('type', typ_tok)
bitstring:add_rule('module', mod_tok(bitstring_ws))
bitstring:add_rule('keyword', key_tok + token(KEY, word_match{
  'binary', 'bytes', 'int', 'float', 'bit_string', 'bits', 'utf8', 'utf16', 'utf32',
  'utf8_codepoint', 'utf16_codepoint', 'utf32_codepoint', 'signed', 'unsigned', 'big', 'little',
  'native', 'unit', 'size'
}))
bitstring:add_rule('function', fn_tok(bitstring_ws))
bitstring:add_rule('label', lab_tok(bitstring_ws))
bitstring:add_rule('identifier', id_tok)
bitstring:add_rule('string', str_tok)
bitstring:add_rule('comment', com_tok)
bitstring:add_rule('number', num_tok)
bitstring:add_rule('operator', op_tok)
bitstring:add_rule('error', err_tok)
lex:embed(bitstring, token(OP, '<<'), token(OP, '>>'))

return lex
