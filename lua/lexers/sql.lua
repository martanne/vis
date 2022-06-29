-- Copyright 2006-2022 Mitchell. See LICENSE.
-- SQL LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('sql')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'add', 'all', 'alter', 'analyze', 'and', 'as', 'asc', 'asensitive', 'before', 'between', 'bigint',
  'binary', 'blob', 'both', 'by', 'call', 'cascade', 'case', 'change', 'char', 'character', 'check',
  'collate', 'column', 'condition', 'connection', 'constraint', 'continue', 'convert', 'create',
  'cross', 'current_date', 'current_time', 'current_timestamp', 'current_user', 'cursor',
  'database', 'databases', 'day_hour', 'day_microsecond', 'day_minute', 'day_second', 'dec',
  'decimal', 'declare', 'default', 'delayed', 'delete', 'desc', 'describe', 'deterministic',
  'distinct', 'distinctrow', 'div', 'double', 'drop', 'dual', 'each', 'else', 'elseif', 'enclosed',
  'escaped', 'exists', 'exit', 'explain', 'false', 'fetch', 'float', 'for', 'force', 'foreign',
  'from', 'fulltext', 'goto', 'grant', 'group', 'having', 'high_priority', 'hour_microsecond',
  'hour_minute', 'hour_second', 'if', 'ignore', 'in', 'index', 'infile', 'inner', 'inout',
  'insensitive', 'insert', 'int', 'integer', 'interval', 'into', 'is', 'iterate', 'join', 'key',
  'keys', 'kill', 'leading', 'leave', 'left', 'like', 'limit', 'lines', 'load', 'localtime',
  'localtimestamp', 'lock', 'long', 'longblob', 'longtext', 'loop', 'low_priority', 'match',
  'mediumblob', 'mediumint', 'mediumtext', 'middleint', 'minute_microsecond', 'minute_second',
  'mod', 'modifies', 'natural', 'not', 'no_write_to_binlog', 'null', 'numeric', 'on', 'optimize',
  'option', 'optionally', 'or', 'order', 'out', 'outer', 'outfile', 'precision', 'primary',
  'procedure', 'purge', 'read', 'reads', 'real', 'references', 'regexp', 'rename', 'repeat',
  'replace', 'require', 'restrict', 'return', 'revoke', 'right', 'rlike', 'schema', 'schemas',
  'second_microsecond', 'select', 'sensitive', 'separator', 'set', 'show', 'smallint', 'soname',
  'spatial', 'specific', 'sql', 'sqlexception', 'sqlstate', 'sqlwarning', 'sql_big_result',
  'sql_calc_found_rows', 'sql_small_result', 'ssl', 'starting', 'straight_join', 'table',
  'terminated', 'text', 'then', 'tinyblob', 'tinyint', 'tinytext', 'to', 'trailing', 'trigger',
  'true', 'undo', 'union', 'unique', 'unlock', 'unsigned', 'update', 'usage', 'use', 'using',
  'utc_date', 'utc_time', 'utc_timestamp', 'values', 'varbinary', 'varchar', 'varcharacter',
  'varying', 'when', 'where', 'while', 'with', 'write', 'xor', 'year_month', 'zerofill'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
local bq_str = lexer.range('`')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + bq_str))

-- Comments.
local line_comment = lexer.to_eol(P('--') + '#')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S(',()')))

return lex
