-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- SQL LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'sql'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = (P('--') + '#') * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local bt_str = l.delimited_range('`')
local string = token(l.STRING, sq_str + dq_str + bt_str)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'add', 'all', 'alter', 'analyze', 'and', 'as', 'asc', 'asensitive', 'before',
  'between', 'bigint', 'binary', 'blob', 'both', 'by', 'call', 'cascade',
  'case', 'change', 'char', 'character', 'check', 'collate', 'column',
  'condition', 'connection', 'constraint', 'continue', 'convert', 'create',
  'cross', 'current_date', 'current_time', 'current_timestamp', 'current_user',
  'cursor', 'database', 'databases', 'day_hour', 'day_microsecond',
  'day_minute', 'day_second', 'dec', 'decimal', 'declare', 'default', 'delayed',
  'delete', 'desc', 'describe', 'deterministic', 'distinct', 'distinctrow',
  'div', 'double', 'drop', 'dual', 'each', 'else', 'elseif', 'enclosed',
  'escaped', 'exists', 'exit', 'explain', 'false', 'fetch', 'float', 'for',
  'force', 'foreign', 'from', 'fulltext', 'goto', 'grant', 'group', 'having',
  'high_priority', 'hour_microsecond', 'hour_minute', 'hour_second', 'if',
  'ignore', 'in', 'index', 'infile', 'inner', 'inout', 'insensitive', 'insert',
  'int', 'integer', 'interval', 'into', 'is', 'iterate', 'join', 'key', 'keys',
  'kill', 'leading', 'leave', 'left', 'like', 'limit', 'lines', 'load',
  'localtime', 'localtimestamp', 'lock', 'long', 'longblob', 'longtext', 'loop',
  'low_priority', 'match', 'mediumblob', 'mediumint', 'mediumtext', 'middleint',
  'minute_microsecond', 'minute_second', 'mod', 'modifies', 'natural', 'not',
  'no_write_to_binlog', 'null', 'numeric', 'on', 'optimize', 'option',
  'optionally', 'or', 'order', 'out', 'outer', 'outfile', 'precision',
  'primary', 'procedure', 'purge', 'read', 'reads', 'real', 'references',
  'regexp', 'rename', 'repeat', 'replace', 'require', 'restrict', 'return',
  'revoke', 'right', 'rlike', 'schema', 'schemas', 'second_microsecond',
  'select', 'sensitive', 'separator', 'set', 'show', 'smallint', 'soname',
  'spatial', 'specific', 'sql', 'sqlexception', 'sqlstate', 'sqlwarning',
  'sql_big_result', 'sql_calc_found_rows', 'sql_small_result', 'ssl',
  'starting', 'straight_join', 'table', 'terminated', 'text', 'then',
  'tinyblob', 'tinyint', 'tinytext', 'to', 'trailing', 'trigger', 'true',
  'undo', 'union', 'unique', 'unlock', 'unsigned', 'update', 'usage', 'use',
  'using', 'utc_date', 'utc_time', 'utc_timestamp', 'values', 'varbinary',
  'varchar', 'varcharacter', 'varying', 'when', 'where', 'while', 'with',
  'write', 'xor', 'year_month', 'zerofill'
}, nil, true))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S(',()'))

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'identifier', identifier},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

return M
