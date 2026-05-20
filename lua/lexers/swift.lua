-- Copyright 2026 Mitchell. See LICENSE.
-- Swift LPeg lexer.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Types.
local constant = lexer.upper * (lexer.upper + '_')^0 * -lexer.alnum
lex:add_rule('type', lex:tag(lexer.TYPE, #lexer.upper * -constant * lexer.word)) -- convention

-- Functions.
local builtin_func = -B('.') *
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word + '`' * lexer.word * '`' +
	('$' * (lexer.digit^1 + lexer.word))))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Strings.
local dq_str = P('#')^0 * lexer.range('"', true) * P('#')^0 -- TODO: balanced #
local ml_str = lexer.range('"""')
local string = lex:tag(lexer.STRING, ml_str + dq_str)
local regex_str = lexer.after_set('+-*%^!=&|?:;,([{<>',
	lexer.range('/', true) + lexer.range('#/', '/#')) -- TODO: multiple, balanced # are allowed
local regex = lex:tag(lexer.REGEX, regex_str)
lex:add_rule('string', string + regex)

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number_('_')))

-- Preprocessor.
lex:add_rule('preproc', lex:tag(lexer.PREPROCESSOR, '#' * lex:word_match(lexer.PREPROCESSOR)))

-- Attributes.
lex:add_rule('attribute', lex:tag(lexer.ANNOTATION,
	'@' * (lex:word_match('attribute') + lexer.upper * lexer.word)))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('+-/*%^~!=&|?:;,.()[]{}<>')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.PREPROCESSOR, '#if', '#endif')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	-- Declarations.
	'associatedtype', 'borrowing', 'class', 'consuming', 'deinit', 'enum', 'extension', 'fileprivate',
	'func', 'import', 'init', 'inout', 'internal', 'let', 'nonisolated', 'open', 'operator',
	'precedencegroup', 'private', 'protocol', 'public', 'rethrows', 'static', 'struct', 'subscript',
	'typealias', 'var',
	-- Statements.
	'break', 'case', 'catch', 'continue', 'default', 'defer', 'do', 'else', 'fallthrough', 'for',
	'guard', 'if', 'in', 'repeat', 'return', 'switch', 'throw', 'where', 'while',
	-- Expressions and types.
	'Any', 'as', 'await', 'catch', 'false', 'is', 'nil', 'rethrows', 'self', 'Self', 'super', 'throw',
	'throws', 'true', 'try',
	-- Patterns.
	'_',
	-- Context-specific.
	'associativity', 'async', 'convenience', 'didSet', 'dynamic', 'final', 'get', 'indirect', 'infix',
	'lazy', 'left', 'mutating', 'none', 'nonmutating', 'optional', 'override', 'package', 'postfix',
	'precedence', 'prefix', 'Protocol', 'required', 'right', 'set', 'some', 'Type', 'unowned', 'weak',
	'willSet'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	-- Numbers and basic values.
	'numericCast', 'all', 'any', 'pointwiseMax', 'pointwiseMin', 'min', 'max', 'abs',
	-- Collections.
	'stride', 'repeatElement', 'sequence', 'zip', 'isKnownUniquelyReferenced',
	-- Input and output.
	'print', 'readLine', 'debugPrint', 'dump', 'assert', 'assertionFailure', 'precondition',
	'preconditionFailure', 'fatalError', 'type',
	-- Concurrency.
	'withTaskGroup', 'withThrowingTaskGroup', 'withDiscardingTaskGroup',
	'withThrowingDiscardingTaskGroup', 'withCheckedContinuation', 'withCheckedThrowingContinuation',
	'withUnsafeContinuation', 'withUnsafeThrowingContinuation', 'withTaskExecutorPreference',
	-- Manual memory management.
	'withUnsafePointer', 'withUnsafeMutablePointer', 'withUnsafeBytes', 'withUnsafeMutableBytes',
	'withUnsafeTemporaryAllocation', 'swap', 'exchange', 'withExtendedLifetime', 'extendLifetime',
	-- Type casting and existential types.
	'withoutActuallyEscaping', 'unsafeDowncast', 'unsafeBitCast',
	-- C interoperability.
	'withVaList', 'getVaList'
})

lex:set_word_list(lexer.PREPROCESSOR, {
	'available', 'colorLiteral', 'else', 'elseif', 'endif', 'fileLiteral', 'if', 'imageLiteral',
	'keyPath', 'selector', 'sourceLocation', 'unavailable'
})

lex:set_word_list('attribute', {
	-- Declaration.
	'available', 'backDeployed', 'discardableResult', 'dynamicCallable', 'dynamicMemberLookup',
	'export', 'freestanding', 'frozen', 'GKInspectable', 'globalActor', 'inlinable', 'main',
	'nonobjc', 'NSApplicationMain', 'NSCopying', 'NSManaged', 'objc', 'objcMembers', 'preconcurrency',
	'propertyWrapper', 'resultBuilder', 'requires_stored_property_inits', 'testable',
	'UIApplicationMain', 'unchecked', 'usableFromInline', 'warn_unqualified_access',
	-- Type.
	'autoclosure', 'convention', 'escaping', 'Sendable',
	-- Switch.
	'unknown'
})

lexer.property['scintillua.comment'] = '//'

return lex
