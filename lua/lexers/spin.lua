-- Copyright 2017-2022 David B. Lamkins <david@lamkins.net>. See LICENSE.
-- Spin LPeg lexer, see https://www.parallax.com/microcontrollers/propeller.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local lex = lexer.new('spin')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  '_clkfreq', '_clkmode', '_free', '_stack', '_xinfreq', 'abort', 'abs', 'absneg', 'add', 'addabs',
  'adds', 'addsx', 'addx', 'and', 'andn', 'byte', 'bytefill', 'bytemove', 'call', 'case', 'chipver',
  'clkfreq', 'clkmode', 'clkset', 'cmp', 'cmps', 'cmpsub', 'cmpsx', 'cmpx', 'cnt', 'cogid',
  'coginit', 'cognew', 'cogstop', 'con', 'constant', 'ctra', 'ctrb', 'dat', 'dira', 'dirb', 'djnz',
  'else', 'elseif', 'elseifnot', 'enc', 'false', 'file', 'fit', 'float', 'from', 'frqa', 'frqb',
  'hubop', 'if', 'ifnot', 'if_a', 'if_ae', 'if_always', 'if_b', 'if_be', 'if_c', 'if_c_and_nz',
  'if_c_and_z', 'if_c_eq_z', 'if_c_ne_z', 'if_c_or_nz', 'if_c_or_z', 'if_e', 'if_nc',
  'if_nc_and_nz', 'if_nc_and_z', 'if_nc_or_nz', 'if_nc_or_z', 'if_ne', 'if_never', 'if_nz',
  'if_nz_and_c', 'if_nz_and_nc', 'if_nz_or_c', 'if_nz_or_nc', 'if_z', 'if_z_and_c', 'if_z_and_nc',
  'if_z_eq_c', 'if_z_ne_c', 'if_z_or_c', 'if_z_or_nc', 'ina', 'inb', 'jmp', 'jmpret', 'lockclr',
  'locknew', 'lockret', 'lockset', 'long', 'longfill', 'longmove', 'lookdown', 'lookdownz',
  'lookup', 'lookupz', 'max', 'maxs', 'min', 'mins', 'mov', 'movd', 'movi', 'movs', 'mul', 'muls',
  'muxc', 'muxnc', 'muxnz', 'muxz', 'neg', 'negc', 'negnc', 'negnz', 'negx', 'negz', 'next', 'nop',
  'not', 'nr', 'obj', 'ones', 'or', 'org', 'other', 'outa', 'outb', 'par', 'phsa', 'phsb', 'pi',
  'pll1x', 'pll2x', 'pll4x', 'pll8x', 'pll16x', 'posx', 'pri', 'pub', 'quit', 'rcfast', 'rcl',
  'rcr', 'rcslow', 'rdbyte', 'rdlong', 'rdword', 'reboot', 'repeat', 'res', 'result', 'ret',
  'return', 'rev', 'rol', 'ror', 'round', 'sar', 'shl', 'shr', 'spr', 'step', 'strcomp', 'string',
  'strsize', 'sub', 'subabs', 'subs', 'subsx', 'subx', 'sumc', 'sumnc', 'sumnz', 'sumz', 'test',
  'testn', 'tjnz', 'tjz', 'to', 'true', 'trunc', 'until', 'var', 'vcfg', 'vscl', 'waitcnt',
  'waitpeq', 'waitpne', 'waitvid', 'wc', 'while', 'word', 'wordfill', 'wordmove', 'wr', 'wrbyte',
  'wrlong', 'wz', 'xinput', 'xor', 'xtal1', 'xtal2', 'xtal3'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true)))

-- Comments.
local line_comment = lexer.to_eol(P("''") + "'")
local block_comment = lexer.range('{', '}')
local block_doc_comment = lexer.range('{{', '}}')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_doc_comment + block_comment))

-- Numbers.
local bin = '%' * S('01_')^1
local ter = '%%' * (R('03') + '_')^1
local hex = '$' * (lexer.xdigit + '_')^1
local dec = (lexer.digit + '_')^1
local int = bin + ter + dec + hex
local rad = P('.') - '..'
local exp = (S('Ee') * S('+-')^-1 * int)^-1
local flt = dec * (rad * dec)^-1 * exp + dec^-1 * rad * dec * exp
lex:add_rule('number', token(lexer.NUMBER, flt + int))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR,
  P('--') + '++' + '^^' + '||' + '~~' + '|<' + '>|' + '@@' + ':=' + '+=' + '-=' + '*=' + '/=' + '**' +
    '**=' + '//' + '//=' + '#>' + '#>=' + '<#' + '<#=' + '~>' + '~>=' + '<<' + '<<=' + '>>' + '>>=' +
    '<-' + '<-=' + '->' + '->=' + '><' + '><=' + '&=' + '|=' + 'and=' + 'or=' + '==' + '===' + '<>' +
    '<>=' + '<=' + '>=' + '=<' + '=<=' + '=>' + '=>=' + '..' + S('+-/*<>~!&=^|?:.()[]@#\\')))

return lex
