-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Verilog LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('verilog')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'always', 'assign', 'begin', 'case', 'casex', 'casez', 'default', 'deassign', 'disable', 'else',
  'end', 'endcase', 'endfunction', 'endgenerate', 'endmodule', 'endprimitive', 'endspecify',
  'endtable', 'endtask', 'for', 'force', 'forever', 'fork', 'function', 'generate', 'if', 'initial',
  'join', 'macromodule', 'module', 'negedge', 'posedge', 'primitive', 'repeat', 'release',
  'specify', 'table', 'task', 'wait', 'while',
  -- Compiler directives.
  '`include', '`define', '`undef', '`ifdef', '`ifndef', '`else', '`endif', '`timescale',
  '`resetall', '`signed', '`unsigned', '`celldefine', '`endcelldefine', '`default_nettype',
  '`unconnected_drive', '`nounconnected_drive', '`protect', '`endprotect', '`protected',
  '`endprotected', '`remove_gatename', '`noremove_gatename', '`remove_netname', '`noremove_netname',
  '`expand_vectornets', '`noexpand_vectornets', '`autoexpand_vectornets',
  -- Signal strengths.
  'strong0', 'strong1', 'pull0', 'pull1', 'weak0', 'weak1', 'highz0', 'highz1', 'small', 'medium',
  'large'
}))

-- Function.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  '$stop', '$finish', '$time', '$stime', '$realtime', '$settrace', '$cleartrace', '$showscopes',
  '$showvars', '$monitoron', '$monitoroff', '$random', '$printtimescale', '$timeformat', '$display',
  -- Built-in primitives.
  'and', 'nand', 'or', 'nor', 'xor', 'xnor', 'buf', 'bufif0', 'bufif1', 'not', 'notif0', 'notif1',
  'nmos', 'pmos', 'cmos', 'rnmos', 'rpmos', 'rcmos', 'tran', 'tranif0', 'tranif1', 'rtran',
  'rtranif0', 'rtranif1', 'pullup', 'pulldown'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match{
  'integer', 'reg', 'time', 'realtime', 'defparam', 'parameter', 'event', 'wire', 'wand', 'wor',
  'tri', 'triand', 'trior', 'tri0', 'tri1', 'trireg', 'vectored', 'scalared', 'input', 'output',
  'inout', 'supply0', 'supply1'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"')))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
local bin_suffix = S('bB') * S('01_xXzZ')^1 * -lexer.xdigit
local oct_suffix = S('oO') * S('01234567_xXzZ')^1
local dec_suffix = S('dD') * S('0123456789_xXzZ')^1
local hex_suffix = S('hH') * S('0123456789abcdefABCDEF_xXzZ')^1
lex:add_rule('number', token(lexer.NUMBER, (lexer.digit + '_')^1 + "'" *
  (bin_suffix + oct_suffix + dec_suffix + hex_suffix)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=~+-/*<>%&|^~,:;()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'case', 'endcase')
lex:add_fold_point(lexer.KEYWORD, 'casex', 'endcase')
lex:add_fold_point(lexer.KEYWORD, 'casez', 'endcase')
lex:add_fold_point(lexer.KEYWORD, 'function', 'endfunction')
lex:add_fold_point(lexer.KEYWORD, 'fork', 'join')
lex:add_fold_point(lexer.KEYWORD, 'table', 'endtable')
lex:add_fold_point(lexer.KEYWORD, 'task', 'endtask')
lex:add_fold_point(lexer.KEYWORD, 'generate', 'endgenerate')
lex:add_fold_point(lexer.KEYWORD, 'specify', 'endspecify')
lex:add_fold_point(lexer.KEYWORD, 'primitive', 'endprimitive')
lex:add_fold_point(lexer.KEYWORD, 'module', 'endmodule')
lex:add_fold_point(lexer.KEYWORD, 'begin', 'end')
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
