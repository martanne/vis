-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Verilog LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'verilog'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '//' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local string = token(l.STRING, l.delimited_range('"'))

-- Numbers.
local bin_suffix = S('bB') * S('01_xXzZ')^1
local oct_suffix = S('oO') * S('01234567_xXzZ')^1
local dec_suffix = S('dD') * S('0123456789_xXzZ')^1
local hex_suffix = S('hH') * S('0123456789abcdefABCDEF_xXzZ')^1
local number = token(l.NUMBER, (l.digit + '_')^1 + "'" *
                               (bin_suffix + oct_suffix + dec_suffix +
                                hex_suffix))

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'always', 'assign', 'begin', 'case', 'casex', 'casez', 'default', 'deassign',
  'disable', 'else', 'end', 'endcase', 'endfunction', 'endgenerate',
  'endmodule', 'endprimitive', 'endspecify', 'endtable', 'endtask', 'for',
  'force', 'forever', 'fork', 'function', 'generate', 'if', 'initial', 'join',
  'macromodule', 'module', 'negedge', 'posedge', 'primitive', 'repeat',
  'release', 'specify', 'table', 'task', 'wait', 'while',
  -- Compiler directives.
  '`include', '`define', '`undef', '`ifdef', '`ifndef', '`else', '`endif',
  '`timescale', '`resetall', '`signed', '`unsigned', '`celldefine',
  '`endcelldefine', '`default_nettype', '`unconnected_drive',
  '`nounconnected_drive', '`protect', '`endprotect', '`protected',
  '`endprotected', '`remove_gatename', '`noremove_gatename', '`remove_netname',
  '`noremove_netname', '`expand_vectornets', '`noexpand_vectornets',
  '`autoexpand_vectornets',
  -- Signal strengths.
  'strong0', 'strong1', 'pull0', 'pull1', 'weak0', 'weak1', 'highz0', 'highz1',
  'small', 'medium', 'large'
}, '`01'))

-- Function.
local func = token(l.FUNCTION, word_match({
  '$stop', '$finish', '$time', '$stime', '$realtime', '$settrace',
  '$cleartrace', '$showscopes', '$showvars', '$monitoron', '$monitoroff',
  '$random', '$printtimescale', '$timeformat', '$display',
  -- Built-in primitives.
  'and', 'nand', 'or', 'nor', 'xor', 'xnor', 'buf', 'bufif0', 'bufif1', 'not',
  'notif0', 'notif1', 'nmos', 'pmos', 'cmos', 'rnmos', 'rpmos', 'rcmos', 'tran',
  'tranif0', 'tranif1', 'rtran', 'rtranif0', 'rtranif1', 'pullup', 'pulldown'
}, '$01'))

-- Types.
local type = token(l.TYPE, word_match({
  'integer', 'reg', 'time', 'realtime', 'defparam', 'parameter', 'event',
  'wire', 'wand', 'wor', 'tri', 'triand', 'trior', 'tri0', 'tri1', 'trireg',
  'vectored', 'scalared', 'input', 'output', 'inout',
  'supply0', 'supply1'
}, '01'))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('=~+-/*<>%&|^~,:;()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'number', number},
  {'keyword', keyword},
  {'function', func},
  {'type', type},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '[%(%){}]', '/%*', '%*/', '//'},
  [l.KEYWORD] = {
    case = 1, casex = 1, casez = 1, endcase = -1, ['function'] = 1,
    endfunction = -1, fork = 1, join = -1, table = 1, endtable = -1, task = 1,
    endtask = -1, generate = 1, endgenerate = -1, specify = 1, endspecify = -1,
    primitive = 1, endprimitive = -1, ['module'] = 1, endmodule = -1, begin = 1,
    ['end'] = -1
  },
  [l.OPERATOR] = {['('] = 1, [')'] = -1, ['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['/*'] = 1, ['*/'] = -1, ['//'] = l.fold_line_comments('//')}
}

return M
