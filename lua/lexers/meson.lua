-- Copyright 2020-2025 Florian Fischer. See LICENSE.
-- Meson file LPeg lexer.

local lexer = lexer
local S = lpeg.S

local lex = lexer.new(..., {fold_by_indentation = true})

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
-- https://mesonbuild.com/Reference-manual.html#builtin-objects
-- https://mesonbuild.com/Reference-manual.html#returned-objects
local method = lex:tag(lexer.FUNCTION_METHOD, lex:word_match(lexer.FUNCTION_METHOD))
-- https://mesonbuild.com/Reference-manual.html#functions
local func = lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
-- A function call must be followed by an opening parenthesis. The matching of function calls
-- instead of just their names is needed to not falsely highlight function names which can also
-- be keyword arguments. For example 'include_directories' can be a function call itself or a
-- keyword argument of an 'executable' or 'library' function call.
lex:add_rule('function', (method + func) * #(lexer.space^0 * '('))

-- Builtin objects.
-- https://mesonbuild.com/Reference-manual.html#builtin-objects
lex:add_rule('object', lex:tag(lexer.VARIABLE_BUILTIN, lex:word_match(lexer.VARIABLE_BUILTIN)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local str = lexer.range("'", true)
local multiline_str = lexer.range("'''")
lex:add_rule('string', lex:tag(lexer.STRING, multiline_str + str))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#', true)))

-- Numbers.
local oct_num = '0o' * lpeg.R('07')
local integer = S('+-')^-1 * (lexer.hex_num + lexer.bin_num + oct_num + lexer.dec_num)
lex:add_rule('number', lex:tag(lexer.NUMBER, integer))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('()[]{}-=+/%:.,?<>')))

-- Word lists
lex:set_word_list(lexer.KEYWORD, {
	'and', 'or', 'not', 'if', 'elif', 'else', 'endif', 'foreach', 'break', 'continue', 'endforeach'
})

lex:set_word_list(lexer.FUNCTION_METHOD, {
	-- array --
	'contains', 'get', 'length',
	-- boolean --
	'to_int', 'to_string',
	-- dictionary --
	'has_key', 'get', 'keys',
	-- disabler --
	'found',
	-- integer --
	'is_even', 'is_odd',
	-- string --
	'contains', 'endswith', 'format', 'join', 'split', 'startswith', 'substring', 'strip', 'to_int',
	'to_lower', 'to_upper', 'underscorify', 'version_compare',
	-- meson object --
	'add_dist_script', 'add_install_script', 'add_postconf_script', 'backend', 'build_root',
	'source_root', 'project_build_root', 'project_source_root', 'current_build_dir',
	'current_source_dir', 'get_compiler', 'get_cross_property', 'get_external_property',
	'can_run_host_binaries', 'has_exe_wrapper', 'install_dependency_manifest', 'is_cross_build',
	'is_subproject', 'is_unity', 'override_find_program', 'override_dependency', 'project_version',
	'project_license', 'project_name', 'version',
	-- *_machine object --
	'cpu_family', 'cpu', 'system', 'endian',
	-- compiler object --
	'alignment', 'cmd_array', 'compiles', 'compute_int', 'find_library', 'first_supported_argument',
	'first_supported_link_argument', 'get_define', 'get_id', 'get_argument_syntax', 'get_linker_id',
	'get_supported_arguments', 'get_supported_link_arguments', 'has_argument', 'has_link_argument',
	'has_function', 'check_header', 'has_header', 'has_header_symbol', 'has_member', 'has_members',
	'has_multi_arguments', 'has_multi_link_arguments', 'has_type', 'links', 'run',
	'symbols_have_underscore_prefix', 'sizeof', 'version', 'has_function_attribute',
	'get_supported_function_attributes',
	-- build target object --
	'extract_all_objects', 'extract_objects', 'full_path', 'private_dir_include', 'name',
	-- configuration data object --
	'get', 'get_unquoted', 'has', 'keys', 'merge_from', 'set', 'set10', 'set_quoted',
	-- custom target object --
	'full_path', 'to_list',
	-- dependency object --
	'found', 'name', 'get_pkgconfig_variable', 'get_configtool_variable', 'type_name', 'version',
	'include_type', 'as_system', 'as_link_whole', 'partial_dependency', 'found',
	-- external program object --
	'found', 'path', 'full_path',
	-- environment object --
	'append', 'prepend', 'set',
	-- external library object --
	'found', 'type_name', 'partial_dependency', 'enabled', 'disabled', 'auto',
	-- generator object --
	'process',
	-- subproject object --
	'found', 'get_variable',
	-- run result object --
	'compiled', 'returncode', 'stderr', 'stdout'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'add_global_arguments', 'add_global_link_arguments', 'add_languages', 'add_project_arguments',
	'add_project_link_arguments', 'add_test_setup', 'alias_targ', 'assert', 'benchmark',
	'both_libraries', 'build_target', 'configuration_data', 'configure_file', 'custom_target',
	'declare_dependency', 'dependency', 'disabler', 'error', 'environment', 'executable',
	'find_library', 'find_program', 'files', 'generator', 'get_option', 'get_variable', 'import',
	'include_directories', 'install_data', 'install_headers', 'install_man', 'install_subdir',
	'is_disabler', 'is_variable', 'jar', 'join_paths', 'library', 'message', 'warning', 'summary',
	'project', 'run_command', 'run_targ', 'set_variable', 'shared_library', 'shared_module',
	'static_library', 'subdir', 'subdir_done', 'subproject', 'test', 'vcs_tag'
})

lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'meson', 'build_machine', 'host_machine', 'target_machine'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {'false', 'true'})

lexer.property['scintillua.comment'] = '#'

return lex
