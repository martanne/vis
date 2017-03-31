-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Prolog LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'prolog'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = '%' * l.nonnewline^0
local block_comment = '/*' * (l.any - '*/')^0 * P('*/')^-1
local comment = token(l.COMMENT, line_comment + block_comment)

-- Strings.
local sq_str = l.delimited_range("'", true)
local dq_str = l.delimited_range('"', true)
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local number = token(l.NUMBER, l.digit^1 * ('.' * l.digit^1)^-1)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  -- Directives, by manual scanning of SWI-Prolog source code
  'abolish', 'arithmetic_function', 'at_halt', 'create_prolog_flag',
  'discontiguous', 'dynamic', 'elif', 'else', 'endif', 'format_predicate', 'if',
  'initialization', 'lazy_list_iterator', 'listing', 'load_extensions',
  'meta_predicate', 'mode', 'module', 'module_transparent', 'multifile', 'op',
  'persistent', 'pop_operators', 'pred', 'predicate_options',
  'prolog_load_context', 'public', 'push_operators', 'record',
  'redefine_system_predicate', 'reexport', 'set_prolog_flag', 'setting',
  'thread_local', 'type', 'use_foreign_library', 'use_module', 'volatile',

  -- Built-in predicates, generated in SWI-Prolog via current_predictate/1.
  'abolish', 'abort', 'absolute_file_name', 'access_file', 'acyclic_term',
  'add_import_module', 'append', 'apply', 'arg', 'assert', 'asserta', 'assertz',
  'at_end_of_stream', 'at_halt', 'atom', 'atom_chars', 'atom_codes',
  'atom_concat', 'atomic', 'atomic_concat', 'atomic_list_concat',
  'atomics_to_string', 'atom_length', 'atom_number', 'atom_prefix',
  'atom_string', 'atom_to_term', 'attach_packs', 'attvar', 'autoload_path',
  'bagof', 'between', 'b_getval', 'blob', 'break', 'b_set_dict', 'b_setval',
  'byte_count', 'call', 'callable', 'call_cleanup', 'call_continuation',
  'call_dcg', 'call_residue_vars', 'call_shared_object_function',
  'call_with_depth_limit', 'call_with_inference_limit', 'cancel_halt', 'catch',
  'character_count', 'char_code', 'char_conversion', 'char_type', 'clause',
  'clause_property', 'close', 'close_shared_object', 'code_type',
  'collation_key', 'compare', 'compile_aux_clauses', 'compile_predicates',
  'compiling', 'compound', 'compound_name_arguments', 'compound_name_arity',
  'consult', 'context_module', 'copy_predicate_clauses', 'copy_stream_data',
  'copy_term', 'copy_term_nat', 'create_prolog_flag',
  'current_arithmetic_function', 'current_atom', 'current_blob',
  'current_char_conversion', 'current_engine', 'current_flag',
  'current_format_predicate', 'current_functor', 'current_input', 'current_key',
  'current_locale', 'current_module', 'current_op', 'current_output',
  'current_predicate', 'current_prolog_flag', 'current_resource',
  'current_signal', 'current_trie', 'cwd', 'cyclic_term', 'date_time_stamp',
  'dcg_translate_rule', 'debugging', 'default_module', 'del_attr', 'del_attrs',
  'del_dict', 'delete_directory', 'delete_file', 'delete_import_module',
  'deterministic', 'dict_create', 'dict_pairs', 'directory_files', 'divmod',
  'downcase_atom', 'duplicate_term', 'dwim_match', 'dwim_predicate',
  'engine_create', 'engine_destroy', 'engine_fetch', 'engine_next',
  'engine_next_reified', 'engine_post', 'engine_self', 'engine_yield',
  'ensure_loaded', 'erase', 'exception', 'exists_directory', 'exists_file',
  'expand_answer', 'expand_file_name', 'expand_file_search_path', 'expand_goal',
  'expand_query', 'expand_term', 'export', 'extern_indirect', 'fail', 'false',
  'fast_read', 'fast_term_serialized', 'fast_write', 'file_base_name',
  'file_directory_name', 'file_name_extension', 'file_search_path',
  'fill_buffer', 'findall', 'findnsols', 'flag', 'float', 'flush_output',
  'forall', 'format', 'format_predicate', 'format_time', 'freeze', 'frozen',
  'functor', 'garbage_collect', 'garbage_collect_atoms',
  'garbage_collect_clauses', 'gc_file_search_cache', 'get0', 'get', 'get_attr',
  'get_attrs', 'get_byte', 'get_char', 'get_code', 'get_dict', 'getenv',
  'get_flag', 'get_single_char', 'get_string_code', 'get_time',
  'goal_expansion', 'ground', 'halt', 'ignore', 'import', 'import_module',
  'instance', 'integer', 'intern_indirect', 'is_absolute_file_name', 'is_dict',
  'is_engine', 'is_list', 'is_stream', 'is_thread', 'keysort', 'known_licenses',
  'leash', 'length', 'library_directory', 'license', 'line_count',
  'line_position', 'load_files', 'locale_create', 'locale_destroy',
  'locale_property', 'make_directory', 'make_library_index', 'memberchk',
  'message_hook', 'message_property', 'message_queue_create',
  'message_queue_destroy', 'message_queue_property', 'message_to_string',
  'module', 'module_property', 'msort', 'mutex_create', 'mutex_destroy',
  'mutex_lock', 'mutex_property', 'mutex_statistics', 'mutex_trylock',
  'mutex_unlock', 'mutex_unlock_all', 'name', 'nb_current', 'nb_delete',
  'nb_getval', 'nb_linkarg', 'nb_link_dict', 'nb_linkval', 'nb_setarg',
  'nb_set_dict', 'nb_setval', 'nl', 'nonvar', 'noprofile', 'noprotocol',
  'normalize_space', 'nospy', 'nospyall', 'not', 'notrace', 'nth_clause',
  'nth_integer_root_and_remainder', 'number', 'number_chars', 'number_codes',
  'number_string', 'numbervars', 'once', 'on_signal', 'op', 'open',
  'open_null_stream', 'open_resource', 'open_shared_object', 'open_string',
  'open_xterm', 'peek_byte', 'peek_char', 'peek_code', 'peek_string', 'phrase',
  'plus', 'portray', 'predicate_option_mode', 'predicate_option_type',
  'predicate_property', 'print', 'print_message', 'print_message_lines',
  'print_toplevel_variables', 'profiler', 'prolog', 'prolog_choice_attribute',
  'prolog_current_choice', 'prolog_current_frame', 'prolog_cut_to',
  'prolog_debug', 'prolog_event_hook', 'prolog_file_type',
  'prolog_frame_attribute', 'prolog_list_goal', 'prolog_load_context',
  'prolog_load_file', 'prolog_nodebug', 'prolog_skip_frame',
  'prolog_skip_level', 'prolog_stack_property', 'prolog_to_os_filename',
  'prompt1', 'prompt', 'protocol', 'protocola', 'protocolling', 'put',
  'put_attr', 'put_attrs', 'put_byte', 'put_char', 'put_code', 'put_dict',
  'pwd', 'qcompile', 'random_property', 'rational', 'read', 'read_clause',
  'read_history', 'read_link', 'read_pending_chars', 'read_pending_codes',
  'read_string', 'read_term', 'read_term_from_atom', 'recorda', 'recorded',
  'recordz', 'redefine_system_predicate', 'reexport', 'reload_library_index',
  'rename_file', 'repeat', 'require', 'reset', 'reset_profiler',
  'residual_goals', 'resource', 'retract', 'retractall', 'same_file',
  'same_term', 'see', 'seeing', 'seek', 'seen', 'select_dict', 'setarg',
  'set_end_of_stream', 'setenv', 'set_flag', 'set_input', 'set_locale',
  'setlocale', 'set_module', 'setof', 'set_output', 'set_prolog_flag',
  'set_prolog_IO', 'set_prolog_stack', 'set_random', 'set_stream',
  'set_stream_position', 'setup_call_catcher_cleanup', 'setup_call_cleanup',
  'shell', 'shift', 'size_file', 'skip', 'sleep', 'sort', 'source_file',
  'source_file_property', 'source_location', 'split_string', 'spy',
  'stamp_date_time', 'statistics', 'stream_pair', 'stream_position_data',
  'stream_property', 'string', 'string_chars', 'string_code', 'string_codes',
  'string_concat', 'string_length', 'string_lower', 'string_upper',
  'strip_module', 'style_check', 'sub_atom', 'sub_atom_icasechk', 'sub_string',
  'subsumes_term', 'succ', 'swiplrc', 'tab', 'tell', 'telling', 'term_attvars',
  'term_expansion', 'term_hash', 'term_string', 'term_to_atom',
  'term_variables', 'text_to_string', 'thread_at_exit', 'thread_create',
  'thread_detach', 'thread_exit', 'thread_get_message', 'thread_join',
  'thread_message_hook', 'thread_peek_message', 'thread_property',
  'thread_self', 'thread_send_message', 'thread_setconcurrency',
  'thread_signal', 'thread_statistics', 'throw', 'time_file', 'tmp_file',
  'tmp_file_stream', 'told', 'trace', 'tracing', 'trie_destroy', 'trie_gen',
  'trie_insert', 'trie_insert_new', 'trie_lookup', 'trie_new', 'trie_property',
  'trie_term', 'trim_stacks', 'true', 'ttyflush', 'tty_get_capability',
  'tty_goto', 'tty_put', 'tty_size', 'unifiable', 'unify_with_occurs_check',
  'unload_file', 'unsetenv', 'upcase_atom', 'use_module', 'var', 'variant_hash',
  'variant_sha1', 'var_number', 'var_property', 'verbose_expansion', 'version',
  'visible', 'wait_for_input', 'wildcard_match', 'with_mutex', 'with_output_to',
  'working_directory', 'write', 'write_canonical', 'write_length', 'writeln',
  'writeq', 'write_term',

  -- Built-in functions, generated in SWI-Prolog via current_arithmetic_function/1.
  'xor', 'rem', 'rdiv', 'mod', 'div', 'abs', 'acos', 'acosh', 'asin', 'asinh',
  'atan2', 'atan', 'atanh', 'ceil', 'ceiling', 'copysign', 'cos', 'cosh',
  'cputime', 'e', 'epsilon', 'erf', 'erfc', 'eval', 'exp', 'float',
  'float_fractional_part', 'float_integer_part', 'floor', 'gcd', 'getbit',
  'inf', 'integer', 'lgamma', 'log10', 'log', 'lsb', 'max', 'min', 'msb',
  'nan', 'pi', 'popcount', 'powm', 'random', 'random_float', 'rational',
  'rationalize', 'round', 'sign', 'sin', 'sinh', 'sqrt', 'tan', 'tanh',
  'truncate',
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('-!+\\|=:;&<>()[]{}'))

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
