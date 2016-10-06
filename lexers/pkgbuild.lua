-- Copyright 2006-2013 gwash. See LICENSE.
-- Archlinux PKGBUILD LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'pkgbuild'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'", false, true)
local dq_str = l.delimited_range('"')
local ex_str = l.delimited_range('`')
local heredoc = '<<' * P(function(input, index)
  local s, e, _, delimiter =
    input:find('(["\']?)([%a_][%w_]*)%1[\n\r\f;]+', index)
  if s == index and delimiter then
    local _, e = input:find('[\n\r\f]+'..delimiter, e)
    return e and e + 1 or #input + 1
  end
end)
local string = token(l.STRING, sq_str + dq_str + ex_str + heredoc)

-- Numbers.
local number = token(l.NUMBER, l.float + l.integer)

-- Keywords.
local keyword = token(l.KEYWORD, word_match({
  'patch', 'cd', 'make', 'patch', 'mkdir', 'cp', 'sed', 'install', 'rm',
  'if', 'then', 'elif', 'else', 'fi', 'case', 'in', 'esac', 'while', 'for',
  'do', 'done', 'continue', 'local', 'return', 'git', 'svn', 'co', 'clone',
  'gconf-merge-schema', 'msg', 'echo', 'ln',
  -- Operators.
  '-a', '-b', '-c', '-d', '-e', '-f', '-g', '-h', '-k', '-p', '-r', '-s', '-t',
  '-u', '-w', '-x', '-O', '-G', '-L', '-S', '-N', '-nt', '-ot', '-ef', '-o',
  '-z', '-n', '-eq', '-ne', '-lt', '-le', '-gt', '-ge', '-Np', '-i'
}, '-'))

-- Functions.
local func = token(l.FUNCTION, word_match{
  'build',
  'check',
  'package',
  'pkgver',
  'prepare'
} * '()')

-- Constants.
local constants = {
  -- We do *not* list pkgver, srcdir and startdir here.
  -- These are defined by makepkg but user should not alter them.
  'arch',
  'backup',
  'changelog',
  'epoch',
  'groups',
  'install',
  'license',
  'noextract',
  'options',
  'pkgbase',
  'pkgdesc',
  'pkgname',
  'pkgrel',
  'pkgver',
  'url',
  'validpgpkeys'
}
local arch_specific = {
  'checkdepends',
  'conflicts',
  'depends',
  'makedepends',
  'md5sums',
  'optdepends',
  'provides',
  'replaces',
  'sha1sums',
  'sha256sums',
  'sha384sums',
  'sha512sums',
  'source'
}
for _, field in ipairs(arch_specific) do
  for _,arch in ipairs({ '', 'i686', 'x86_64' }) do
    table.insert(constants, field..(arch ~= '' and '_'..arch or ''))
  end
end
local constant = token(l.CONSTANT, word_match(constants))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Variables.
local variable = token(l.VARIABLE,
                       '$' * (S('!#?*@$') +
                       l.delimited_range('()', true, true) +
                       l.delimited_range('[]', true, true) +
                       l.delimited_range('{}', true, true) +
                       l.delimited_range('`', true, true) +
                       l.digit^1 + l.word))

-- Operators.
local operator = token(l.OPERATOR, S('=!<>+-/*^~.,:;?()[]{}'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'keyword', keyword},
  {'function', func},
  {'constant', constant},
  {'identifier', identifier},
  {'variable', variable},
  {'operator', operator},
}

M._foldsymbols = {
  _patterns = {'[%(%){}]', '#'},
  [l.OPERATOR] = {['('] = 1, [')'] = -1, ['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
