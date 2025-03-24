-- Copyright 2022-2025 Matej Cepl mcepl.att.cepl.eu. See LICENSE.

local lexer = lexer
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local lex = lexer.new(...)

-- Comments.
lex:add_rule('comment', lex:tag(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers (including versions)
lex:add_rule('number', lex:tag(lexer.NUMBER,
	lpeg.B(lexer.space) * (lexer.number * (lexer.number + S('.+~'))^0)))

-- Operators
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('&<>=|')))

-- Strings.
lex:add_rule('string', lex:tag(lexer.STRING, lexer.range('"')))

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, (lex:word_match(lexer.KEYWORD) +
	(P('Patch') + 'Source') * R('09')^0) * ':'))

-- Macros
lex:add_rule('command', lex:tag(lexer.FUNCTION, lexer.range(S('%$')^1 * S('{')^0 *
	((lexer.alnum + S('_?'))^0), S('}')^0)))

-- Constants
lex:add_rule('constant', lex:tag(lexer.CONSTANT, lex:word_match(lexer.CONSTANT)))

-- Word lists
lex:set_word_list(lexer.CONSTANT, {
	'rhel', 'fedora', 'suse_version', 'sle_version', 'x86_64', 'aarch64', 'ppc64le', 'riscv64',
	's390x'
})

lex:set_word_list(lexer.KEYWORD, {
	'Prereq', 'Summary', 'Name', 'Version', 'Packager', 'Requires', 'Recommends', 'Suggests',
	'Supplements', 'Enhances', 'Icon', 'URL', 'Source', 'Patch', 'Prefix', 'Packager', 'Group',
	'License', 'Release', 'BuildRoot', 'Distribution', 'Vendor', 'Provides', 'ExclusiveArch',
	'ExcludeArch', 'ExclusiveOS', 'Obsoletes', 'BuildArch', 'BuildArchitectures', 'BuildRequires',
	'BuildConflicts', 'BuildPreReq', 'Conflicts', 'AutoRequires', 'AutoReq', 'AutoReqProv',
	'AutoProv', 'Epoch'
})

lexer.property['scintillua.comment'] = '#'

return lex
