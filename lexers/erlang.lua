-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Erlang LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'erlang'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '%' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"')
local literal = '$' * l.any * l.alnum^0
local string = token(l.STRING, sq_str + dq_str + literal)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'after', 'begin', 'case', 'catch', 'cond', 'end', 'fun', 'if', 'let', 'of',
  'query', 'receive', 'when',
  -- Operators.
  'div', 'rem', 'or', 'xor', 'bor', 'bxor', 'bsl', 'bsr', 'and', 'band', 'not',
  'bnot',
  'badarg', 'nocookie', 'false', 'true'
})

-- Functions.
local func = token(l.FUNCTION, word_match{
  'abs', 'alive', 'apply', 'atom_to_list', 'binary_to_list', 'binary_to_term',
  'concat_binary', 'date', 'disconnect_node', 'element', 'erase', 'exit',
  'float', 'float_to_list', 'get', 'get_keys', 'group_leader', 'halt', 'hd',
  'integer_to_list', 'is_alive', 'length', 'link', 'list_to_atom',
  'list_to_binary', 'list_to_float', 'list_to_integer', 'list_to_pid',
  'list_to_tuple', 'load_module', 'make_ref', 'monitor_node', 'node', 'nodes',
  'now', 'open_port', 'pid_to_list', 'process_flag', 'process_info', 'process',
  'put', 'register', 'registered', 'round', 'self', 'setelement', 'size',
  'spawn', 'spawn_link', 'split_binary', 'statistics', 'term_to_binary',
  'throw', 'time', 'tl', 'trunc', 'tuple_to_list', 'unlink', 'unregister',
  'whereis',
  -- Others.
  'atom', 'binary', 'constant', 'function', 'integer', 'list', 'number', 'pid',
  'ports', 'port_close', 'port_info', 'reference', 'record',
  -- Erlang:.
  'check_process_code', 'delete_module', 'get_cookie', 'hash', 'math',
  'module_loaded', 'preloaded', 'processes', 'purge_module', 'set_cookie',
  'set_node',
  -- Math.
  'acos', 'asin', 'atan', 'atan2', 'cos', 'cosh', 'exp', 'log', 'log10', 'pi',
  'pow', 'power', 'sin', 'sinh', 'sqrt', 'tan', 'tanh'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('-<>.;=/|#+*:,?!()[]{}'))

-- Directives.
local directive = token('directive', '-' * word_match{
  'author', 'compile', 'copyright', 'define', 'doc', 'else', 'endif', 'export',
  'file', 'ifdef', 'ifndef', 'import', 'include_lib', 'include', 'module',
  'record', 'undef'
})

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'function', func},
  {'identifier', identifier},
  {'directive', directive},
  {'string', string},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._tokenstyles = {
  directive = l.STYLE_PREPROCESSOR
}

M._foldsymbols = {
  _patterns = {'[a-z]+', '[%(%)%[%]{}]', '%%'},
  [l.KEYWORD] = {
    case = 1, fun = 1, ['if'] = 1, query = 1, receive = 1, ['end'] = -1
  },
  [l.OPERATOR] = {
    ['('] = 1, [')'] = -1, ['['] = 1, [']'] = -1, ['{'] = 1, ['}'] = -1
  },
  [l.COMMENT] = {['%'] = l.fold_line_comments('%')}
}

return M
