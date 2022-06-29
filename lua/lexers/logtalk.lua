-- Copyright © 2017-2022 Michael T. Richter <ttmrichter@gmail.com>. See LICENSE.
-- Logtalk LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('logtalk', {inherit = lexer.load('prolog')})

-- add logtalk keywords to prolog ones
local directives = {
  'set_logtalk_flag', 'object', 'info', 'built_in', 'threaded', 'uses', 'alias', 'use_module',
  'coinductive', 'export', 'reexport', 'public', 'metapredicate', 'mode', 'meta_non_terminal',
  'protected', 'synchronized', 'private', 'module', 'if', 'elif', 'else', 'endif', 'category',
  'protocol', 'end_object', 'end_category', 'end_protocol', 'meta_predicate'
}
local indent = token(lexer.WHITESPACE, lexer.starts_line(S(' \t')^1))^-1
lex:modify_rule('directive',
  (indent * token(lexer.OPERATOR, ':-') * token(lexer.WHITESPACE, S(' \t')^0) *
    token(lexer.PREPROCESSOR, word_match(directives))
) + lex:get_rule('directive'))

-- Whitespace.
lex:modify_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

local zero_arity_keywords = {
  -- extracted from test document in logtalk distribution
  'comment', 'argnames', 'arguments', 'author', 'version', 'date', 'parameters', 'parnames',
  'copyright', 'license', 'remarks', 'see_also', 'as', 'logtalk_make', 'instantiation_error',
  'system_error'
}
local one_plus_arity_keywords = {
  -- extracted from test document in logtalk distribution
  'implements', 'imports', 'extends', 'instantiates', 'specializes', 'number_chars', 'number_code',
  'current_category', 'current_object', 'current_protocol', 'create_category', 'create_object',
  'create_protocol', 'abolish_category', 'abolish_object', 'abolish_protocol', 'category_property',
  'object_property', 'protocol_property', 'extends_category', 'extends_object', 'extends_protocol',
  'implements_protocol', 'imports_category', 'instantiates_class', 'specializes_class',
  'complements_object', 'conforms_to_protocol', 'abolish_events', 'current_event', 'define_events',
  'threaded', 'threaded_call', 'threaded_call', 'threaded_once', 'threaded_ignore', 'threaded_exit',
  'threaded_peek', 'threaded_cancel', 'threaded_wait', 'threaded_notify', 'threaded_engine',
  'threaded_engine_create', 'threaded_engine_destroy', 'threaded_engine_self',
  'threaded_engine_next', 'threaded_engine_next_reified', 'threaded_engine_yield',
  'threaded_engine_post', 'threaded_engine_fetch', 'logtalk_compile', 'logtalk_load',
  'logtalk_library_path', 'logtalk_load_context', 'logtalk_make_target_action',
  'current_logtalk_flag', 'set_logtalk_flag', 'create_logtalk_flag', 'context', 'parameter', 'self',
  'sender', 'this', 'type_error', 'domain_error', 'existence_error', 'permission_error',
  'representation_error', 'evaluation_error', 'resource_error', 'syntax_error', 'bagof', 'findall',
  'forall', 'setof', 'before', 'after', 'forward', 'phrase', 'expand_term', 'expand_goal',
  'term_expansion', 'goal_expansion', 'numbervars', 'put_code', 'put_byte', 'current_op', 'op',
  'ignore', 'repeat', 'number_codes', 'current_prolog_flag', 'set_prolog_flag', 'keysort', 'sort'
}
local keyword = word_match(zero_arity_keywords) + (word_match(one_plus_arity_keywords) * #P('('))
lex:modify_rule('keyword', token(lexer.KEYWORD, keyword) + lex:get_rule('keyword'))

local operators = {
  -- extracted from test document in logtalk distribution
  'as'
}
lex:modify_rule('operator', token(lexer.OPERATOR, word_match(operators)) + lex:get_rule('operator'))

return lex
