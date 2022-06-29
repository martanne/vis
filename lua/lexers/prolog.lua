-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Lexer enhanced to conform to the realities of Prologs on the ground by
-- Michael T. Richter.  Copyright is explicitly assigned back to Mitchell.
-- Prolog LPeg lexer.

--[[
  Prologs are notoriously fractious with many barely-compatible dialects.  To
  make Textadept more useful for these cases, directives and keywords are
  grouped by dialect.  Selecting a dialect is a simple matter of setting the
  buffer/lexer property "prolog.dialect" in init.lua.  Dialects currently in
  the lexer file are:
    - 'iso': the generic ISO standard without modules.
    - 'gprolog': GNU Prolog.
    - 'swipl': SWI-Prolog.

  The default dialect is 'iso' if none is defined.  (You probably don't want
  this.)

  Note that there will be undoubtedly duplicated entries in various categories
  because of the flexibility of Prolog and the automated tools used to gather
  most information.  This is not an issue, however, because directives override
  arity-0 predicates which override arity-1+ predicates which override bifs
  which override operators.
]]

local lexer = require('lexer')

local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('prolog')

local dialects = setmetatable({gprolog = 'gprolog', swipl = 'swipl'},
  {__index = function(_, _) return 'iso' end})
local dialect = dialects[lexer.property['prolog.dialect']]

-- Directives.
local directives = {}
directives.iso = [[
  -- Gathered by inspection of GNU Prolog documentation.
  dynamic multifile discontiguous include ensure_loaded op char_conversion
  set_prolog_flag initialization
]]
directives.gprolog = directives.iso .. [[
  -- Gathered by inspection of GNU Prolog documentation.
  public ensure_linked built_in if else endif elif foreign
]]
directives.swipl = directives.iso .. [[
  -- Gathered by liberal use of grep on the SWI source and libraries.
  coinductive current_predicate_option expects_dialect http_handler listen
  module multifile use_foreign_library use_module dynamic http_handler
  initialization json_object multifile record use_module abolish
  arithmetic_function asserta at_halt begin_tests chr_constraint chr_option
  chr_type clear_cache constraints consult create_prolog_flag
  current_prolog_flag debug discontiguous dynamic elif else encoding end_tests
  endif expects_dialect export forall format format_predicate html_meta
  html_resource http_handler http_request_expansion if include
  init_color_term_flag init_options initialization json_object
  lazy_list_iterator license listen load_extensions load_files
  load_foreign_library meta_predicate mode module module_transparent multifile
  noprofile op pce_begin_class pce_end_class pce_global pce_group persistent
  pop_operators pred predicate_options print_message prolog_load_context prompt
  public push_hprolog_library push_ifprolog_library, push_operators
  push_sicstus_library push_xsb_library push_yap_library, quasi_quotation_syntax
  record redefine_system_predicate reexport register_iri_scheme residual_goals
  retract set_module set_prolog_flag set_script_dir set_test_options setenv
  setting style_check table thread_local thread_local	message type
  use_class_template use_foreign_library use_module utter volatile build_schema
  chr_constraint chr_option chr_type cql_option determinate discontiguous
  dynamic endif format_predicate if initialization license meta_predicate mode
  module multifile op reexport thread_local use_module volatile
]]
lex:add_rule('directive',
  token(lexer.WHITESPACE, lexer.starts_line(S(' \t'))^0) *
  token(lexer.OPERATOR, ':-') *
  token(lexer.WHITESPACE, S(' \t')^0) *
  token(lexer.PREPROCESSOR, word_match(directives[dialect])))

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
local zero_arity_keywords = {}
zero_arity_keywords.iso = [[
  -- eyeballed from GNU Prolog documentation
  true fail pi float_overflow int_overflow int_underflow undefined asserta
  assertz retract retractall clause abolish current_predicate findall bagof
  setof at_end_of_stream flush_output nl halt false
]]
zero_arity_keywords.gprolog = [[
  -- Collected automatically via current_predicate/1 with some cleanup.
  at_end_of_stream wam_debug listing flush_output fail told false top_level
  shell trace debugging seen repeat abort nl statistics halt notrace randomize
  true nospyall nodebug debug stop break
]]
zero_arity_keywords.swipl = [[
  -- Collected automatically via current_predicate/1 with some cleanup.
  noprotocol compiling ttyflush true abort license known_licenses
  print_toplevel_variables initialize mutex_statistics break reset_profiler
  win_has_menu version prolog abolish_nonincremental_tables false halt undefined
  abolish_all_tables reload_library_index garbage_collect repeat nospyall
  tracing trace notrace trim_stacks garbage_collect_clauses
  garbage_collect_atoms mutex_unlock_all seen told nl debugging fail
  at_end_of_stream attach_packs flush_output true
]]
local one_plus_arity_keywords = {}
one_plus_arity_keywords.iso = [[
  -- eyeballed from GNU Prolog documentation
  call catch throw var nonvar atom integer float number atomic compound
  callable ground unify_with_occurs_check compare functor arg copy_term
  term_variables subsumes_term acyclic_term predicate_property current_input
  current_output set_input set_output open close current_stream stream_property
  set_stream_position get_char get_code is peek_char peek_code put_char putcode
  get_byte peek_byte read_term read write_term write writeq write_canonical
  char_conversion current_char_conversion call once repeat atom_length
  atom_concat sub_atom char_code atom_chars atom_codes
]]
one_plus_arity_keywords.gprolog = [[
  -- Collected automatically via current_predicate/1 with some cleanup.
  abolish absolute_file_name acyclic_term add_linedit_completion
  add_stream_alias add_stream_mirror append architecture arg argument_counter
  argument_list argument_value asserta assertz at_end_of_stream atom atom_chars
  atom_codes atom_concat atom_length atom_property atomic bagof between
  bind_variables call call_det call_with_args callable catch change_directory
  char_code char_conversion character_count clause close close_input_atom_stream
  close_input_chars_stream close_input_codes_stream close_output_atom_stream
  close_output_chars_stream close_output_codes_stream compare compound consult
  copy_term cpu_time create_pipe current_alias current_atom current_bip_name
  current_char_conversion current_input current_mirror current_op current_output
  current_predicate current_prolog_flag current_stream date_time
  decompose_file_name delete delete_directory delete_file directory_files
  display display_to_atom display_to_chars display_to_codes environ exec
  expand_term fd_all_different fd_at_least_one fd_at_most_one fd_atleast
  fd_atmost fd_cardinality fd_dom fd_domain fd_domain_bool fd_element
  fd_element_var fd_exactly fd_has_extra_cstr fd_has_vector fd_labeling
  fd_labelingff fd_max fd_max_integer fd_maximize fd_min fd_minimize
  fd_not_prime fd_only_one fd_prime fd_reified_in fd_relation fd_relationc
  fd_set_vector_max fd_size fd_use_vector fd_var fd_vector_max file_exists
  file_permission file_property find_linedit_completion findall flatten float
  flush_output for forall fork_prolog format format_to_atom format_to_chars
  format_to_codes functor g_array_size g_assign g_assignb g_dec g_deco g_inc
  g_inco g_link g_read g_reset_bit g_set_bit g_test_reset_bit g_test_set_bit
  generic_var get get_byte get_char get_code get_key get_key_no_echo
  get_linedit_prompt get_print_stream get_seed get0 ground halt host_name
  hostname_address integer is_absolute_file_name is_list is_relative_file_name
  keysort last last_read_start_line_column leash length line_count line_position
  list list_or_partial_list listing load lower_upper make_directory maplist
  max_list member memberchk min_list msort name name_query_vars
  name_singleton_vars new_atom nl non_fd_var non_generic_var nonvar nospy nth
  nth0 nth1 number number_atom number_chars number_codes numbervars once op open
  open_input_atom_stream open_input_chars_stream open_input_codes_stream
  open_output_atom_stream open_output_chars_stream open_output_codes_stream
  os_version partial_list peek_byte peek_char peek_code permutation phrase popen
  portray_clause predicate_property prefix print print_to_atom print_to_chars
  print_to_codes prolog_file_name prolog_pid put put_byte put_char put_code
  random read read_atom read_from_atom read_from_chars read_from_codes
  read_integer read_number read_pl_state_file read_term read_term_from_atom
  read_term_from_chars read_term_from_codes read_token read_token_from_atom
  read_token_from_chars read_token_from_codes real_time remove_stream_mirror
  rename_file retract retractall reverse see seeing seek select send_signal
  set_bip_name set_input set_linedit_prompt set_output set_prolog_flag set_seed
  set_stream_buffering set_stream_eof_action set_stream_line_column
  set_stream_position set_stream_type setarg setof shell skip sleep socket
  socket_accept socket_bind socket_close socket_connect socket_listen sort spawn
  spy spypoint_condition sr_change_options sr_close sr_current_descriptor
  sr_error_from_exception sr_get_error_counters sr_get_file_name
  sr_get_include_list sr_get_include_stream_list sr_get_module sr_get_position
  sr_get_size_counters sr_get_stream sr_new_pass sr_open sr_read_term
  sr_set_error_counters sr_write_error sr_write_message statistics
  stream_line_column stream_position stream_property sub_atom sublist
  subsumes_term subtract succ suffix sum_list syntax_error_info system
  system_time tab tell telling temporary_file temporary_name term_hash term_ref
  term_variables throw unget_byte unget_char unget_code unify_with_occurs_check
  unlink user_time var wait working_directory write write_canonical
  write_canonical_to_atom write_canonical_to_chars write_canonical_to_codes
  write_pl_state_file write_term write_term_to_atom write_term_to_chars
  write_term_to_codes write_to_atom write_to_chars write_to_codes writeq
  writeq_to_atom writeq_to_chars writeq_to_codes
]]
one_plus_arity_keywords.swipl = [[
  -- Collected automatically via current_predicate/1 with some cleanup.
  prolog_exception_hook term_expansion expand_answer message_property resource
  help goal_expansion file_search_path prolog_clause_name thread_message_hook
  prolog_file_type goal_expansion prolog_predicate_name exception writeln
  term_expansion expand_query url_path message_hook library_directory resource
  portray prolog_load_file prolog_list_goal ansi_format source_file_property
  asserta call_dcg source_location wait_for_input locale_destroy set_locale
  read_pending_codes thread_join open_dde_conversation win_folder protocol
  copy_stream_data current_locale read_pending_chars win_add_dll_directory
  protocola thread_property win_shell goal_expansion phrase gc_file_search_cache
  dcg_translate_rule protocolling win_registry_get_value term_expansion
  dcg_translate_rule assert copy_stream_data once bagof prompt1 tnot assertz
  phrase sort ignore thread_statistics assert locale_create
  win_remove_dll_directory term_expansion read_term asserta clause assertz
  predicate_option_type is_thread get_single_char set_prolog_IO expand_goal
  ground message_queue_create locale_property close_dde_conversation
  goal_expansion clause zipper_open_new_file_in_zip term_to_atom with_output_to
  module expand_term redefine_system_predicate thread_detach dde_execute
  term_string read_clause compile_predicates predicate_option_mode noprofile
  read_term_from_atom cancel_halt non_terminal atom_to_term line_position frozen
  dde_request findnsols prolog_skip_level prolog_current_choice get get_attrs
  license var_property nb_delete unwrap_predicate zipper_open_current put_attrs
  dde_poke set_stream read_term zip_file_info_ memberchk seek expand_goal get0
  call var integer attach_packs byte_count zipper_goto findnsols character_count
  expand_term get_flag atom line_count set_flag atomic tab create_prolog_flag
  copy_term import_module verbose_expansion b_setval duplicate_term
  prolog_load_context attach_packs prolog_listen b_getval prolog_frame_attribute
  prompt copy_term_nat nb_linkval tab prolog_choice_attribute set_prolog_flag
  nb_getval prolog_skip_frame del_attrs skip sort license open_null_stream
  nb_current prolog_listen msort is_list is_stream get keysort win_shell
  prolog_unlisten notrace get0 add_import_module wildcard_match profiler
  delete_directory trie_gen_compiled expand_file_name file_name_extension
  delete_file writeq win_module_file call write get_dict win_exec
  directory_files trie_insert make_directory engine_next_reified del_dict sleep
  getenv call_continuation trie_gen_compiled prolog_to_os_filename
  is_absolute_file_name trie_insert engine_fetch engine_create strip_module call
  delete_import_module write_canonical compile_aux_clauses setenv callable
  is_engine write_term call set_module call halt catch findall trie_gen
  trie_destroy rename_file shift unify_with_occurs_check engine_yield forall
  unsetenv trie_term file_directory_name version current_engine file_base_name
  engine_self import trie_gen trie_lookup write_term trie_update freeze
  engine_post export put_dict same_file trie_new call trie_delete start_tabling
  is_trie residual_goals thread_peek_message thread_get_message dict_pairs
  set_end_of_stream call_cleanup current_predicate arg dict_create
  thread_setconcurrency read_link is_dict at_halt tmp_file not put_dict
  setup_call_cleanup abolish_nonincremental_tables time_file
  start_subsumptive_tabling char_conversion compound sub_atom access_file call
  call_cleanup abolish nonvar current_functor abolish_module_tables
  subsumes_term engine_post call retractall compare engine_next prolog_cut_to
  size_file current_char_conversion predicate_property nonground engine_destroy
  message_queue_property format abolish qcompile thread_send_message stream_pair
  message_queue_create same_term number select_dict catch_with_backtrace
  thread_get_message thread_send_message win_insert_menu_item message_queue_set
  <meta-call> exists_directory copy_term nb_set_dict prolog_nodebug functor
  current_table cyclic_term untable read exists_file thread_peek_message
  b_set_dict engine_create prolog_debug acyclic_term writeln get_dict
  compound_name_arity abolish_table_subgoals start_tabling trie_insert
  nb_link_dict message_queue_destroy thread_get_message is_dict nth_clause
  absolute_file_name term_singletons make_library_index set_output retract
  context_module current_trie term_attvars load_files get_char ensure_loaded
  current_input prolog_current_frame make_library_index term_variables
  compound_name_arguments reexport autoload_path get_code set_input flag
  thread_create use_module findall thread_join call_with_inference_limit
  var_number dwim_match consult peek_code close nospy print_message
  term_variables trie_property read_history get_byte default_module get_byte
  print on_signal get_char call_residue_vars dwim_match atom_prefix unifiable
  use_module numbervars load_files get_code open format_time
  copy_predicate_clauses reexport leash current_output sub_string close
  format_time atom_codes stamp_date_time require name open_shared_object open
  atom_chars current_predicate format tmp_file_stream term_hash rational
  source_file reset atom_concat atom_length current_prolog_flag rational
  dwim_predicate date_time_stamp stream_property string_upper setlocale format
  writeln current_module normalize_space writeq current_flag shell upcase_atom
  qcompile char_code atomic_concat read string_lower write term_string
  numbervars working_directory number_codes set_prolog_gc_thread downcase_atom
  format_predicate number_string open_shared_object style_check char_type print
  stream_position_data code_type write_canonical number_chars length
  current_arithmetic_function atomic_list_concat del_attr read_string zip_unlock
  open_resource string_length zip_lock see erase open_resource setof
  atomic_list_concat current_format_predicate current_resource with_mutex
  atomics_to_string term_hash absolute_file_name deterministic current_atom
  thread_create collation_key get_attr variant_hash string_concat atom_number
  put put_attr variant_sha1 thread_signal mutex_unlock tty_size current_key
  mutex_create fill_buffer expand_file_search_path blob shell
  register_iri_scheme skip fast_read divmod mutex_trylock thread_self put
  mutex_property fast_write mutex_lock current_blob sub_atom_icasechk
  mutex_destroy fast_term_serialized split_string set_stream_position recorda
  telling setarg thread_exit zip_open_stream instance mutex_create statistics
  append get_time zip_close_ tell atomics_to_string clause_property attvar
  zip_clone seeing nth_integer_root_and_remainder recorda put_byte string_chars
  spy recordz print_message_lines current_op put_char nl source_file
  string_codes op setup_call_catcher_cleanup nb_linkarg recorded put_code
  peek_byte apply module_property atom_string nb_setarg succ recordz
  message_to_string close_shared_object peek_char between recorded visible plus
  call_shared_object_function peek_code peek_byte set_prolog_stack float throw
  at_end_of_stream get_string_code call_with_depth_limit random_property
  flush_output peek_string open_xterm peek_char open_string string_code
  set_random prolog_stack_property put_char unload_file nb_setval put_byte
  current_signal put_code write_length string read_string text_to_string
]]
lex:add_rule('keyword', token(lexer.KEYWORD,
  word_match(zero_arity_keywords[dialect]) +
  word_match(one_plus_arity_keywords[dialect]) * #P('(')))

-- BIFs.
local bifs = {}
bifs.iso = [[
  -- eyeballed from GNU Prolog documentation
  xor abs sign min max sqrt tan atan atan2 cos acos sin asin exp log float
  ceiling floor round truncate float_fractional_part float_integer_part rem div
  mod
]]
bifs.gprolog = bifs.iso .. [[
  -- eyeballed from GNU Prolog documentation
  inc dec lsb msb popcount gcd tanh atanh cosh acosh sinh asinh log10 rnd
]]
bifs.swipl = [[
  -- Collected automatically via current_arithmetic_function/1 with some
  -- cleanup.
  abs acos acosh asinh atan atan atanh atan2 ceil ceiling copysign cos cosh
  cputime div getbit e epsilon erf erfc eval exp float float_fractional_part
  float_integer_part floor gcd inf integer lgamma log log10 lsb max min mod msb
  nan pi popcount powm random random_float rational rationalize rdiv rem round
  sign sin sinh sqrt tan tanh truncate xor
]]
lex:add_rule('bif', token(lexer.FUNCTION, word_match(bifs[dialect]) * #P('(')))

-- Numbers.
local decimal_group = S('+-')^-1 * (lexer.digit + '_')^1
local binary_number = '0b' * (S('01') + '_')^1
local character_code = '0\'' * S('\\')^-1 * lexer.graph
local decimal_number = decimal_group * ('.' * decimal_group)^-1 *
  ('e' * decimal_group)^-1
local hexadecimal_number = '0x' * (lexer.xdigit + '_')^1
local octal_number = '0o' * (S('01234567') + '_')^1
lex:add_rule('number', token(lexer.NUMBER, character_code + binary_number +
  hexadecimal_number + octal_number + decimal_number))

-- Comments.
local line_comment = lexer.to_eol('%')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Operators.
local operators = {}
operators.iso = [[
  -- Collected automatically via current_op/3 with some cleanup and comparison
  -- to docs.
  rem div mod is
]]
operators.gprolog = operators.iso -- GNU Prolog's textual operators are the same
operators.swipl = [[
  -- Collected automatically via current_op/3 with some cleanup.
  is as volatile mod discontiguous div rdiv meta_predicate public xor
  module_transparent multifile table dynamic thread_initialization thread_local
  initialization rem
]]
lex:add_rule('operator', token(lexer.OPERATOR, word_match(operators[dialect]) +
  S('-!+\\|=:;&<>()[]{}/*^@?.')))

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, (lexer.upper + '_') *
  (lexer.word^1 + lexer.digit^1 + P('_')^1)^0))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

return lex
