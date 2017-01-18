-- Copyright 2017 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- Spin LPeg lexer, see https://www.parallax.com/microcontrollers/propeller

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'spin'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = (P("''") + P("'")) * l.nonnewline^0
local block_comment = P('{') * (l.any - P('}'))^0 * P('}')^-1
local block_doc_comment = P('{{') * (l.any - P('}}'))^0 * P('}}')^-1
local comment = token(l.COMMENT, line_comment + block_doc_comment + block_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"', true))

-- Numbers.
local bin = '%' * S('01_')^1
local ter = P('%%') * (R('03') + P('_'))^1
local hex = P('$') * (R('09') + R('af') + R('AF') + P('_'))^1
local dec = (R('09') + P('_'))^1
local int = bin + ter + dec + hex
local rad = P('.') - P('..')
local exp = (S('Ee') * S('+-')^-1 * int)^-1
local flt = dec * (rad * dec)^-1 * exp + dec^-1 * rad * dec * exp
local number = token(l.NUMBER, flt + int)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  '_clkfreq', '_clkmode', '_free', '_stack', '_xinfreq', 'abort', 'abs',
  'absneg', 'add', 'addabs', 'adds', 'addsx', 'addx', 'and', 'andn', 'byte',
  'bytefill', 'bytemove', 'call', 'case', 'chipver', 'clkfreq', 'clkmode',
  'clkset', 'cmp', 'cmps', 'cmpsub', 'cmpsx', 'cmpx', 'cnt', 'cogid',
  'coginit', 'cognew', 'cogstop', 'con', 'constant', 'ctra', 'ctrb', 'dat',
  'dira', 'dirb', 'djnz', 'else', 'elseif', 'elseifnot', 'enc', 'false',
  'file', 'fit', 'float', 'from', 'frqa', 'frqb', 'hubop', 'if', 'ifnot',
  'if_a', 'if_ae', 'if_always', 'if_b', 'if_be', 'if_c', 'if_c_and_nz',
  'if_c_and_z', 'if_c_eq_z', 'if_c_ne_z', 'if_c_or_nz', 'if_c_or_z', 'if_e',
  'if_nc', 'if_nc_and_nz', 'if_nc_and_z', 'if_nc_or_nz', 'if_nc_or_z',
  'if_ne', 'if_never', 'if_nz', 'if_nz_and_c', 'if_nz_and_nc', 'if_nz_or_c',
  'if_nz_or_nc', 'if_z', 'if_z_and_c', 'if_z_and_nc', 'if_z_eq_c',
  'if_z_ne_c', 'if_z_or_c', 'if_z_or_nc', 'ina', 'inb', 'jmp', 'jmpret',
  'lockclr', 'locknew', 'lockret', 'lockset', 'long', 'longfill', 'longmove',
  'lookdown', 'lookdownz', 'lookup', 'lookupz', 'max', 'maxs', 'min', 'mins',
  'mov', 'movd', 'movi', 'movs', 'mul', 'muls', 'muxc', 'muxnc', 'muxnz',
  'muxz', 'neg', 'negc', 'negnc', 'negnz', 'negx', 'negz', 'next', 'nop',
  'not', 'nr', 'obj', 'ones', 'or', 'org', 'other', 'outa', 'outb', 'par',
  'phsa', 'phsb', 'pi', 'pll1x', 'pll2x', 'pll4x', 'pll8x', 'pll16x', 'posx',
  'pri', 'pub', 'quit', 'rcfast', 'rcl', 'rcr', 'rcslow', 'rdbyte', 'rdlong',
  'rdword', 'reboot', 'repeat', 'res', 'result', 'ret', 'return', 'rev',
  'rol', 'ror', 'round', 'sar', 'shl', 'shr', 'spr', 'step', 'strcomp',
  'string', 'strsize', 'sub', 'subabs', 'subs', 'subsx', 'subx', 'sumc',
  'sumnc', 'sumnz', 'sumz', 'test', 'testn', 'tjnz', 'tjz', 'to', 'true',
  'trunc', 'until', 'var', 'vcfg', 'vscl', 'waitcnt', 'waitpeq', 'waitpne',
  'waitvid', 'wc', 'while', 'word', 'wordfill', 'wordmove', 'wr', 'wrbyte',
  'wrlong', 'wz', 'xinput', 'xor', 'xtal1', 'xtal2', 'xtal3'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local punct = S('+-/*<>~!&=^|?:.()[]@#\\')
local dec = P('--')
local inc = P('++')
local sqrt = P('^^')
local abs = P('||')
local sexw = P('~~')
local decode = P('|<')
local encode = P('>|')
local objref = P('@@')
local assign = P(':=')
local plus_a = P('+=')
local minus_a = P('-=')
local mull_a = P('*=')
local div_a = P('/=')
local mulu = P('**')
local mulu_a = P('**=')
local mod = P('//')
local mod_a = P('//=')
local limmin = P('#>')
local limmin_a = P('#>=')
local limmax = P('<#')
local limmax_a = P('<#=')
local sar = P('~>')
local sar_a = P('~>=')
local shl = P('<<')
local shl_a = P('<<=')
local shr = P('>>')
local shr_a = P('>>=')
local rol = P('<-')
local rol_a = P('<-=')
local ror = P('->')
local ror_a = P('->=')
local rev = P('><')
local rev_a = P('><=')
local band_a = P('&=')
local bor_a = P('|=')
local sand_a = P('and=')
local sor_a = P('or=')
local equal = P('==')
local equal_a = P('===')
local nequal = P('<>')
local nequal_a = P('<>=')
local less_a = P('<=')
local greater_a = P('>=')
local leq = P('=<')
local leq_a = P('=<=')
local geq = P('=>')
local geq_a = P('=>=')
local dots = P('..')
local operator = token(l.OPERATOR, dec + inc + sqrt + abs + sexw +
  decode + encode + objref + assign + plus_a + minus_a + mull_a + div_a +
  mulu + mulu_a + mod + mod_a + limmin + limmin_a + limmax + limmax_a +
  sar + sar_a + shl + shl_a + shr + shr_a + rol + rol_a + ror + ror_a +
  rev + rev_a + band_a + bor_a + sand_a + sor_a + equal + equal_a +
  nequal + nequal_a + less_a + greater_a + leq + leq_a + geq + geq_a +
  dots + punct)

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'keyword', keyword},
  {'number', number},
  {'operator', operator},
  {'identifier', identifier},
  {'string', string},
}

return M
