-- Copyright 2006-2025 Mitchell. See LICENSE.
-- C++ LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
local basic_type = lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE))
local stl_type = lex:tag(lexer.TYPE .. '.stl', 'std::' * lex:word_match(lexer.TYPE .. '.stl'))
lex:add_rule('type', basic_type + stl_type * -P('::'))

-- Functions.
local non_member = -(B('.') + B('->') + B('::'))
local builtin_func = lex:tag(lexer.FUNCTION_BUILTIN,
	P('std::')^-1 * lex:word_match(lexer.FUNCTION_BUILTIN))
local stl_func = lex:tag(lexer.FUNCTION_BUILTIN .. '.stl',
	'std::' * lex:word_match(lexer.FUNCTION_BUILTIN .. '.stl'))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = (B('.') + B('->')) * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function',
	(non_member * (stl_func + builtin_func) + method + func) * #(lexer.space^0 * '('))

-- Constants.
local const =
	lex:tag(lexer.CONSTANT_BUILTIN, P('std::')^-1 * lex:word_match(lexer.CONSTANT_BUILTIN))
local stl_const = lex:tag(lexer.CONSTANT_BUILTIN .. '.stl',
	'std::' * lex:word_match(lexer.CONSTANT_BUILTIN .. '.stl'))
lex:add_rule('constants', stl_const + const)

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
lex:add_rule('string', lex:tag(lexer.STRING, ('u8' + S('LuU'))^-1 * (sq_str + dq_str)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_("'")))

-- Preprocessor.
local include = lex:tag(lexer.PREPROCESSOR, '#' * S('\t ')^0 * 'include') *
	(lex:get_rule('whitespace') * lex:tag(lexer.STRING, lexer.range('<', '>', true)))^-1
local preproc = lex:tag(lexer.PREPROCESSOR, '#' * S('\t ')^0 * lex:word_match(lexer.PREPROCESSOR))
lex:add_rule('preprocessor', include + preproc)

-- Attributes.
lex:add_rule('attribute', lex:tag(lexer.ATTRIBUTE, '[[' * lex:word_match(lexer.ATTRIBUTE) * ']]'))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;,.()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.PREPROCESSOR, 'if', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifdef', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifndef', 'endif')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'asm', 'auto', 'break', 'case', 'catch', 'class', 'const', 'const_cast', 'continue', 'default',
	'delete', 'do', 'dynamic_cast', 'else', 'explicit', 'export', 'extern', 'false', 'for', 'friend',
	'goto', 'if', 'inline', 'mutable', 'namespace', 'new', 'operator', 'private', 'protected',
	'public', 'register', 'reinterpret_cast', 'return', 'sizeof', 'static', 'static_cast', 'switch',
	'template', 'this', 'throw', 'true', 'try', 'typedef', 'typeid', 'typename', 'using', 'virtual',
	'volatile', 'while',
	-- Operators.
	'and', 'and_eq', 'bitand', 'bitor', 'compl', 'not', 'not_eq', 'or', 'or_eq', 'xor', 'xor_eq',
	-- C++11.
	'alignas', 'alignof', 'constexpr', 'decltype', 'final', 'noexcept', 'nullptr', 'override',
	'static_assert', 'thread_local', --
	'consteval', 'constinit', 'co_await', 'co_return', 'co_yield', 'requires' -- C++20
})

lex:set_word_list(lexer.TYPE, {
	'bool', 'char', 'double', 'enum', 'float', 'int', 'long', 'short', 'signed', 'struct', 'union',
	'unsigned', 'void', 'wchar_t', --
	'char16_t', 'char32_t', -- C++11
	'char8_t', -- C++20
	-- <cstddef>
	'size_t', 'ptrdiff_t', 'max_align_t', --
	'byte', -- C++17
	-- <cstdint>
	'int8_t', 'int16_t', 'int32_t', 'int64_t', 'int_fast8_t', 'int_fast16_t', 'int_fast32_t',
	'int_fast64_t', 'int_least8_t', 'int_least16_t', 'int_least32_t', 'int_least64_t', 'intmax_t',
	'intptr_t', 'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'uint_fast8_t', 'uint_fast16_t',
	'uint_fast32_t', 'uint_fast64_t', 'uint_least8_t', 'uint_least16_t', 'uint_least32_t',
	'uint_least64_t', 'uintmax_t', 'uintptr_t'
})

lex:set_word_list(lexer.TYPE .. '.stl', {
	'any', 'bad_any_cast', -- <any> C++17
	'array', -- <array> C++11
	'atomic', -- <atomic> C++11
	'barrier', -- <barrier> C++20
	'bitset', -- <bitset>
	-- <concepts> C++20
	'same_as', 'derived_from', 'convertible_to', 'common_reference_with', 'common_with', 'integral',
	'signed_integral', 'unsigned_integral', 'floating_point', 'assignable_from', 'swappable',
	'swappable_with', 'destructible', 'constructible_from', 'default_initializable',
	'move_constructible', 'copy_constructible', 'equality_comparable', 'equality_comparable_with',
	'movable', 'copyable', 'semiregular', 'regular', 'invocable', 'regular_invocable', 'predicate',
	'relation', 'equivalence_relation', 'strict_weak_order', --
	'complex', -- <complex>
	'deque', -- <deque>
	'exception', 'bad_exception', -- <exception>
	'forward_list', -- <forward_list> C++11
	'function', 'hash', -- <functional> C++11
	-- <future> C++11
	'promise', 'packaged_task', 'future', 'shared_future', 'launch', 'future_status', 'future_error',
	'future_errc', --
	'initializer_list', -- <initializer_list>
	'istream', 'iostream', -- <istream>
	-- <iterator>
	'reverse_iterator', 'back_insert_iterator', 'front_insert_iterator', 'insert_iterator',
	'istream_iterator', 'ostream_iterator', 'istreambuf_iterator', 'ostreambuf_iterator', --
	'move_iterator', -- C++11
	'latch', -- <latch> C++20
	'list', -- <list>
	-- <map>
	'map', 'multimap', --
	'unordered_set', 'unordered_map', 'unordered_multiset', 'unordered_multimap', -- C++11
	'unique_ptr', 'shared_ptr', 'weak_ptr', -- <memory> C++11
	-- <mutex> C++11
	'mutex', 'timed_mutex', 'recursive_mutex', 'recursive_timed_mutex', 'lock_guard', 'unique_lock', --
	'scoped_lock', -- C++17
	'optional', 'bad_optional_access', -- <optional> C++17
	'ostream', -- <ostream>
	'queue', 'priority_queue', -- <queue>
	-- <random> C++11
	'linear_congruential_engine', 'mersenne_twister_engine', 'subtract_with_carry_engine',
	'discard_block_engine', 'independent_bits_engine', 'shuffle_order_engine', 'random_device',
	'uniform_int_distribution', 'uniform_real_distribution', 'bernoulli_distribution',
	'binomial_distribution', 'negative_binomial_distribution', 'geometric_distribution',
	'poisson_distribution', 'exponential_distribution', 'gamma_distribution', 'weibull_distribution',
	'extreme_value_distribution', 'normal_distribution', 'lognormal_distribution',
	'chi_squared_distribution', 'cauchy_distribution', 'fisher_f_distribution',
	'student_t_distribution', 'discrete_distibution', 'piecewise_constant_distribution',
	'piecewise_linear_distribution', 'seed_seq', --
	'ratio', -- <ratio> C++11
	-- <regex> C++11
	'regex', 'csub_match', 'ssub_match', 'cmatch', 'smatch', 'cregex_iterator', 'sregex_iterator',
	'cregex_token_iterator', 'sregex_token_iterator', 'regex_error', 'regex_traits', --
	'counting_semaphore', 'binary_semaphore', -- <semaphore> C++20
	'set', 'multiset', -- <set>
	'span', -- <span> C++20
	'stringbuf', 'istringstream', 'ostringstream', 'stringstream', -- <stringstream>
	'stack', -- <stack>
	-- <stdexcept>
	'logic_error', 'invalid_argument', 'domain_error', 'length_error', 'out_of_range',
	'runtime_error', 'range_error', 'overflow_error', 'underflow_error', --
	'streambuf', -- <streambuf>
	-- <string>
	'string', --
	'u16string', 'u32string', -- C++11
	'u8string', -- C++20
	-- <string_view> C++17
	'string_view', 'u16string_view', 'u32string_view', --
	'u8string_view', -- C++20
	'syncbuf', 'osyncstream', -- <syncstream> C++20
	'thread', -- <thread> C++11
	'tuple', 'tuple_size', 'tuple_element', -- <tuple> C++11
	'pair', -- <utility>
	'variant', 'monostate', 'bad_variant_access', 'variant_size', 'variant_alternative', -- <variant> C++17
	'vector' -- <vector>
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'assert', -- <cassert>
	-- <cctype>
	'isalnum', 'isalpha', 'islower', 'isupper', 'isdigit', 'isxdigit', 'iscntrl', 'isgraph',
	'isspace', 'isprint', 'ispunct', 'tolower', 'toupper', --
	'isblank', -- C++11
	'va_start', 'va_arg', 'va_end', -- <cstdarg>
	-- <cmath>
	'abs', 'fmod', 'exp', 'log', 'log10', 'pow', 'sqrt', 'sin', 'cos', 'tan', 'asin', 'acos', 'atan',
	'atan2', 'sinh', 'cosh', 'tanh', 'ceil', 'floor', 'frexp', 'ldexp', 'modf',
	-- C++11.
	'remainder', 'remquo', 'exp2', 'expm1', 'log2', 'log1p', 'cbrt', 'hypot', 'asinh', 'acosh',
	'atanh', 'erf', 'erfc', 'tgamma', 'lgamma', 'trunc', 'round', 'nearbyint', 'rint', 'scalbn',
	'ilogb', 'logb', 'nextafter', 'copysign', 'isfinite', 'isinf', 'isnan', 'isnormal', 'signbit',
	'isgreater', 'isgreaterequal', 'isless', 'islessequal', 'islessgreater', 'isunordered', --
	-- C++17.
	'assoc_laguerre', 'assoc_legendre', 'beta', 'comp_ellint_1', 'comp_ellint_2', 'comp_ellint_3',
	'cyl_bessel_i', 'cyl_bessel_j', 'cyl_bessel_k', 'cyl_neumann', 'ellint_1', 'ellint_2', 'ellint_3',
	'expint', 'lhermite', 'lgendre', 'laguerre', 'riemann_zeta', 'sph_bessel', 'sph_legendre',
	'sph_neumann', --
	'lerp', -- C++20
	-- <cstring>
	'strcpy', 'strncpy', 'strcat', 'strncat', 'strxfrm', 'strlen', 'strcmp', 'strncmp', 'strcoll',
	'strchr', 'strrchr', 'strspn', 'strcspn', 'strpbrk', 'strstr', 'strtok', 'memchr', 'memcmp',
	'memset', 'memcpy', 'memmove', 'strerror'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN .. '.stl', {
	-- <algorithm>
	'for_each', 'count', 'count_if', 'mismatch', 'find', 'find_if', 'find_end', 'find_first_of',
	'adjacent_find', 'search', 'search_n', 'copy', 'copy_backward', 'fill', 'fill_n', 'transform',
	'generate', 'generate_n', 'remove', 'remove_if', 'remove_copy', 'remove_copy_if', 'replace',
	'replace_if', 'replace_copy', 'replace_copy_if', 'swap', 'swap_ranges', 'iter_swap', 'reverse',
	'reverse_copy', 'rotate', 'rotate_copy', 'unique_copy', 'partition', 'stable_partition', 'sort',
	'partial_sort', 'partial_sort_copy', 'stable_sort', 'nth_element', 'lower_bound', 'upper_bound',
	'binary_search', 'equal_range', 'merge', 'inplace_merge', 'includes', 'set_difference',
	'set_intersection', 'set_symmetric_difference', 'set_union', 'make_heap', 'push_heap', 'pop_heap',
	'sort_heap', 'max', 'max_element', 'min', 'min_element', 'equal', 'lexicographical_compare',
	'next_permutation', 'prev_permutation', --
	-- C++11.
	'all_of', 'any_of', 'none_of', 'find_if_not', 'copy_if', 'copy_n', 'move', 'move_backward',
	'shuffle', 'is_partitioned', 'partition_copy', 'partition_point', 'is_sorted', 'is_sorted_until',
	'is_heap', 'is_heap_until', 'minmax', 'minmax_element', 'is_permutation', --
	'for_each_n', 'random_shuffle', 'sample', 'clamp', -- C++17
	'shift_left', 'shift_right', 'lexicographical_compare_three_way', -- C++20
	'make_any', 'any_cast', -- <any> C++17
	-- <bit> C++20
	'bit_cast', 'byteswap', 'has_single_bit', 'bit_ceil', 'bit_floor', 'bit_width', 'rotl', 'rotr',
	'countl_zero', 'countl_one', 'countl_zero', 'countr_one', 'popcount', --
	'from_chars', 'to_chars', -- <charconv> C++17
	-- <format> C++20
	'format', 'format_to', 'format_to_n', 'formatted_size', 'vformat', 'vformat_to',
	'visit_format_arg', 'make_format_args', --
	'async', 'future_category', -- <future> C++11
	-- <iterator>
	'front_inserter', 'back_inserter', 'inserter', --
	'make_move_iterator', -- C++11
	'make_reverse_iterator', -- C++14
	-- <memory>
	'make_shared', 'allocate_shared', 'static_pointer_cast', 'dynamic_pointer_cast',
	'const_pointer_cast', --
	'make_unique', -- C++14
	'reinterpret_pointer_cast', -- C++17
	'try_lock', 'lock', 'call_once', -- <mutex> C++11
	-- <numeric>
	'accumulate', 'inner_product', 'adjacent_difference', 'partial_sum', --
	'iota', -- C++11
	'reduce', 'transform_reduce', 'inclusive_scan', 'exclusive_scan', 'gcd', 'lcm', -- C++17
	'midpoint', -- C++20
	'make_optional', -- <optional> C++17
	'generate_canonical', -- <random> C++11
	'regex_match', 'regex_search', 'regex_replace', -- <regex> C++11
	'as_bytes', 'as_writable_bytes', -- <span> C++20
	-- <tuple> C++11
	'make_tuple', 'tie', 'forward_as_tuple', 'tuple_cat', --
	'apply', 'make_from_tuple', -- C++17
	-- <utility>
	'swap', 'make_pair', 'get', --
	'forward', 'move', 'move_if_noexcept', 'declval', -- C++11
	'exchange', -- C++14
	'as_const', -- C++17
	-- C++20.
	'cmp_equal', 'cmp_not_equal', 'cmp_less', 'cmp_greater', 'cmp_less_equal', 'cmp_greater_equal',
	'in_range', --
	'visit', 'holds_alternative', 'get_if' -- <variant> C++17
})

lex:set_word_list(lexer.CONSTANT_BUILTIN .. '.stl', {
	'cin', 'cout', 'cerr', 'clog', -- <iostream>
	'endl', 'ends', 'flush', -- <ostream>
	'nullopt' -- <optional> C++17
})

lex:set_word_list(lexer.PREPROCESSOR, {
	'define', 'defined', 'elif', 'else', 'endif', 'error', 'if', 'ifdef', 'ifndef', 'import', 'line',
	'pragma', 'undef', 'using', 'warning', --
	'export', 'include', 'module' -- C++20
})

lex:set_word_list(lexer.ATTRIBUTE, {
	'carries_dependency', 'noreturn', -- C++11
	'deprecated', -- C++14
	'fallthrough', 'maybe_unused', 'nodiscard', -- C++17
	'likely', 'no_unique_address', 'unlikely' -- C++20
})

lexer.property['scintillua.comment'] = '//'

return lex
