-- GDScript LPeg lexer.

local lexer = lexer
local token, word_match = lexer.token, lexer.word_match
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(..., {fold_by_indentation = true})

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
local builtin_func = -B('.') *
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Types.
lex:add_rule('type', lex:tag(lexer.TYPE, lex:word_match(lexer.TYPE)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN,
	lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Annotations.
lex:add_rule('annotation', lex:tag(lexer.ANNOTATION, '@' * lexer.word))

-- Strings.
local sq_str = lexer.range("'", true)
local dq_str = lexer.range('"', true)
local tq_str = lexer.range("'''") + lexer.range('"""')
local str_prefix = (S('rR') + S('&^'))^-1
lex:add_rule('string', lex:tag(lexer.STRING, str_prefix * (tq_str + sq_str + dq_str)))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#', true)))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_')))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('!%^&*()[]{}-=+/|:;.,<>~')))

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'and', 'as', 'assert', 'await', 'break', 'breakpoint', 'class', 'class_name', 'const',
	'continue', 'elif', 'else', 'enum', 'extends', 'for', 'func', 'get', 'if', 'in', 'is', 'match',
	'not', 'or', 'pass', 'preload', 'return', 'self', 'set', 'signal', 'static', 'super', 'trait',
	'var', 'void', 'when', 'while', 'yield'
})

lex:set_word_list(lexer.TYPE, {
	'bool', 'int', 'float', 'String', 'StringName', 'NodePath', 'Vector2', 'Vector2i', 'Vector3',
	'Vector3i', 'Vector4', 'Vector4i', 'Rect2', 'Rect2i', 'Transform2D', 'Transform3D', 'Plane',
	'Quaternion', 'AABB', 'Basis', 'Projection', 'Color', 'RID', 'Object', 'Callable', 'Signal',
	'Dictionary', 'Array', 'PackedByteArray', 'PackedInt32Array', 'PackedInt64Array',
	'PackedFloat32Array', 'PackedFloat64Array', 'PackedStringArray', 'PackedVector2Array',
	'PackedVector3Array', 'PackedColorArray', 'PackedVector4Array', 'Variant', 'void',
	'RefCounted', 'Resource', 'Script', 'PackedScene', 'Texture', 'Texture2D', 'Material', 'Shader',
	'InputEvent', 'Viewport', 'SceneTree', 'Node', 'CanvasItem', 'Node2D', 'Node3D', 'Control'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'abs', 'absf', 'absi', 'acos', 'acosh', 'angle_difference', 'asin', 'asinh', 'atan', 'atan2',
	'atanh', 'bezier_derivative', 'bezier_interpolate', 'bytes_to_var', 'bytes_to_var_with_objects',
	'ceil', 'ceilf', 'ceili', 'clamp', 'clampf', 'clampi', 'cos', 'cosh', 'cubic_interpolate',
	'cubic_interpolate_angle', 'cubic_interpolate_angle_in_time', 'cubic_interpolate_in_time',
	'db_to_linear', 'deg_to_rad', 'ease', 'error_string', 'exp', 'floor', 'floorf', 'floori',
	'fmod', 'fposmod', 'hash', 'instance_from_id', 'inverse_lerp', 'is_equal_approx', 'is_finite',
	'is_inf', 'is_instance_id_valid', 'is_instance_valid', 'is_nan', 'is_same', 'is_zero_approx',
	'lerp', 'lerp_angle', 'lerpf', 'linear_to_db', 'log', 'max', 'maxf', 'maxi', 'min', 'minf',
	'mini', 'move_toward', 'nearest_po2', 'pingpong', 'posmod', 'pow', 'print', 'print_rich',
	'print_verbose', 'printerr', 'printraw', 'prints', 'printt', 'push_error', 'push_warning',
	'rad_to_deg', 'rand_from_seed', 'randf', 'randf_range', 'randfn', 'randi', 'randi_range',
	'randomize', 'remap', 'rid_allocate_id', 'rid_from_int64', 'rotate_toward', 'round', 'roundf',
	'roundi', 'seed', 'sign', 'signf', 'signi', 'sin', 'sinh', 'smoothstep', 'snapped', 'snappedf',
	'snappedi', 'sqrt', 'step_decimals', 'str', 'str_to_var', 'tan', 'tanh', 'type_convert',
	'type_string', 'typeof', 'var_to_bytes', 'var_to_bytes_with_objects', 'var_to_str', 'weakref',
	'wrap', 'wrapf', 'wrapi'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'true', 'false', 'null', 'PI', 'TAU', 'INF', 'NAN'
})

lexer.property['scintillua.comment'] = '#'

return lex
