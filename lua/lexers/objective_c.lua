-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Objective C LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('objective_c')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- From C.
  'asm', 'auto', 'break', 'case', 'const', 'continue', 'default', 'do', 'else', 'extern', 'false',
  'for', 'goto', 'if', 'inline', 'register', 'return', 'sizeof', 'static', 'switch', 'true',
  'typedef', 'void', 'volatile', 'while', 'restrict', '_Bool', '_Complex', '_Pragma', '_Imaginary',
  -- Objective C.
  'oneway', 'in', 'out', 'inout', 'bycopy', 'byref', 'self', 'super',
  -- Preprocessor directives.
  '@interface', '@implementation', '@protocol', '@end', '@private', '@protected', '@public',
  '@class', '@selector', '@encode', '@defs', '@synchronized', '@try', '@throw', '@catch',
  '@finally',
  -- Constants.
  'TRUE', 'FALSE', 'YES', 'NO', 'NULL', 'nil', 'Nil', 'METHOD_NULL'
}))

-- Types.
lex:add_rule('type', token(lexer.TYPE, word_match(
  'apply_t id Class MetaClass Object Protocol retval_t SEL STR IMP BOOL TypedStream')))

-- Strings.
local sq_str = P('L')^-1 * lexer.range("'", true)
local dq_str = P('L')^-1 * lexer.range('"', true)
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol('//', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Preprocessor.
lex:add_rule('preprocessor',
  #lexer.starts_line('#') * token(lexer.PREPROCESSOR, '#' * S('\t ')^0 * word_match{
    'define', 'elif', 'else', 'endif', 'error', 'if', 'ifdef', 'ifndef', 'import', 'include',
    'line', 'pragma', 'undef', 'warning'
  }))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-/*%<>!=^&|?~:;.()[]{}')))

-- Fold symbols.
lex:add_fold_point(lexer.PREPROCESSOR, 'region', 'endregion')
lex:add_fold_point(lexer.PREPROCESSOR, 'if', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifdef', 'endif')
lex:add_fold_point(lexer.PREPROCESSOR, 'ifndef', 'endif')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))

return lex
