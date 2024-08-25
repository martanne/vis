-- Copyright 2006-2024 Mitchell. See LICENSE. 
-- Copyright 2024 Brett Mahar. See LICENSE.
-- Odin LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Functions.
local builtin_func = -lpeg.B('.') *
  lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = lpeg.B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local raw_str = lexer.range('`', false, false)
lex:add_rule('string', lex:tag(lexer.STRING, sq_str + dq_str + raw_str))

-- Comments.
local line_comment = lexer.to_eol('//')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number * P('i')^-1))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('@?+-#*/$%&|^<>=!~:;.,()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
  -- keyword control
  'asm', 'yield', 'await', 'using', 'do', 'inline', 'no_inline', 'fallthrough', 'break', 'continue', 
  'case', 'vector', 'static', 'dynamic', 'atomic', 'push_allocator', 'push_context', 'if', 'else', 
  'when', 'for', 'in', 'not_in', 'defer', 'switch', 'return', 'const', 'import', 'export', 'foreign', 
  'package', 'import_load', 'foreign_library', 'foreign_system_library', 'or_else', 'or_return',
  'or_break','or_continue', 'where', 'expect', 'syscall',
  -- keyword operator
  'distinct', 'context',
  -- keyword function
  'size_of', 'align_of', 'offset_of', 'type_of', 'type_info', 'type_info_of', 'typeid_of', 'identifier', 
  'cast', 'transmute', 'auto_cast', 'down_cast', 'union_cast', 'accumulator', 'offset_of_selector',
  'offset_of_member', 'offset_of_by_string', 'swizzle', 'min', 'max', 'abs', 'clamp', 
  'is_package_imported', 'sqrt', 'valgrind_client_request'  
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, 'true false nil iota')

lex:set_word_list(lexer.TYPE, {
  -- type
  'i8', 'i16', 'i32', 'i64', 'i128', 'int', 'u8', 'u32', 'u64', 'u128', 'uint', 'uintptr', 
  'f16', 'f32', 'f64', 'complex32', 'complex64', 'complex128', 'bool', 'b8', 'b16', 'b32', 'b64',
  'string', 'rune', 'rawptr', 'any', 'byte', 'cstring', 'complex', 'quaternion', 'real', 'imag',
  'jmag', 'kmag', 'conj', 
  -- storage type 
  'type', 'var', 'dynamic', 'struct', 'enum', 'union', 'map', 'set', 'bit_set', 'bit_field', 'typeid', 
  'matrix', 'rawunion', 'proc', 'macro', 'soa_struct' 
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
  'len', 'cap', 'make', 'resize', 'reserve', 'append', 'delete', 'assertf?', 'panicf?', 
  -- package annotations
  'thread_local', 'test', 'static', 'require_target_feature', 'require_results', 'require', 'private', 
  'optimization_mode', 'objc_type', 'objc_name', 'objc_is_class_method', 'objc_class', 
  'no_instrumentation', 'linkage', 'link_prefix', 'link_name', 'instrumentation_enter', 
  'instrumentation_exit', 'init', 'fini', 'extra_linker_flags', 'disabled', 'entry_point_only', 
  'enable_target_feature', 'enable_target_features', 'deprecated', 'deferred_out', 'deferred_in_out', 
  'deferred_in', 'builtin', 'cold', 'default_calling_convention'  
})

lexer.property['scintillua.comment'] = '//'

return lex
