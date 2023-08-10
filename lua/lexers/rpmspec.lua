-- Copyright 2022-2024 Matej Cepl mcepl.att.cepl.eu. See LICENSE.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('rpmspec')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"')))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'Prereq', 'Summary', 'Name', 'Version', 'Packager', 'Requires', 'Recommends', 'Suggests',
  'Supplements', 'Enhances', 'Icon', 'URL', 'Source', 'Patch', 'Prefix', 'Packager', 'Group',
  'License', 'Release', 'BuildRoot', 'Distribution', 'Vendor', 'Provides', 'ExclusiveArch',
  'ExcludeArch', 'ExclusiveOS', 'Obsoletes', 'BuildArch', 'BuildArchitectures', 'BuildRequires',
  'BuildConflicts', 'BuildPreReq', 'Conflicts', 'AutoRequires', 'AutoReq', 'AutoReqProv',
  'AutoProv', 'Epoch'
}))

-- Macros
lex:add_rule('command', token(lexer.FUNCTION, '%' * lexer.word))

lexer.property['scintillua.comment'] = '#'

return lex
