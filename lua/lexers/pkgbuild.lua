-- Copyright 2006-2022 gwash. See LICENSE.
-- Archlinux PKGBUILD LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('pkgbuild')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Strings.
local sq_str = lexer.range("'", false, false)
local dq_str = lexer.range('"')
local ex_str = lexer.range('`')
local heredoc = '<<' * P(function(input, index)
  local s, e, _, delimiter = input:find('(["\']?)([%a_][%w_]*)%1[\n\r\f;]+', index)
  if s == index and delimiter then
    e = select(2, input:find('[\n\r\f]+' .. delimiter, e))
    return e and e + 1 or #input + 1
  end
end)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str + ex_str + heredoc))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'patch', 'cd', 'make', 'patch', 'mkdir', 'cp', 'sed', 'install', 'rm', 'if', 'then', 'elif',
  'else', 'fi', 'case', 'in', 'esac', 'while', 'for', 'do', 'done', 'continue', 'local', 'return',
  'git', 'svn', 'co', 'clone', 'gconf-merge-schema', 'msg', 'echo', 'ln',
  -- Operators.
  '-a', '-b', '-c', '-d', '-e', '-f', '-g', '-h', '-k', '-p', '-r', '-s', '-t', '-u', '-w', '-x',
  '-O', '-G', '-L', '-S', '-N', '-nt', '-ot', '-ef', '-o', '-z', '-n', '-eq', '-ne', '-lt', '-le',
  '-gt', '-ge', '-Np', '-i'
}))

-- Functions.
lex:add_rule('function',
  token(lexer.FUNCTION, word_match('build check package pkgver prepare') * '()'))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match{
  -- We do *not* list pkgver srcdir and startdir here.
  -- These are defined by makepkg but user should not alter them.
  'arch', 'backup', 'changelog', 'checkdepends', 'conflicts', 'depends', 'epoch', 'groups',
  'install', 'license', 'makedepends', 'md5sums', 'noextract', 'optdepends', 'options', 'pkgbase',
  'pkgdesc', 'pkgname', 'pkgrel', 'pkgver', 'provides', 'replaces', 'sha1sums', 'sha256sums',
  'sha384sums', 'sha512sums', 'source', 'url', 'validpgpkeys'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Variables.
local symbol = S('!#?*@$')
local parens = lexer.range('(', ')', true)
local brackets = lexer.range('[', ']', true)
local braces = lexer.range('{', '}', true)
local backticks = lexer.range('`', true, false)
local number = lexer.dec_num
lex:add_rule('variable', token(lexer.VARIABLE, '$' *
  (symbol + parens + brackets + braces + backticks + number + lexer.word)))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('=!<>+-/*^~.,:;?()[]{}')))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
