-- Copyright 2006-2022 Mitchell. See LICENSE.
-- YAML LPeg lexer.
-- It does not keep track of indentation perfectly.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new('yaml', {fold_by_indentation = true})

-- Whitespace.
local indent = #lexer.starts_line(S(' \t')) *
  (token(lexer.WHITESPACE, ' ') + token('indent_error', '\t'))^1
lex:add_rule('indent', indent)
lex:add_style('indent_error', {back = lexer.colors.red})
lex:add_rule('whitespace', token(lexer.WHITESPACE, S(' \t')^1 + lexer.newline^1))

-- Keys.
local word = (lexer.alpha + '-' * -lexer.space) * (lexer.alnum + '-')^0
lex:add_rule('key', token(lexer.KEYWORD, word * (S(' \t_')^1 * word^-1)^0) * #(':' * lexer.space))

-- Constants.
lex:add_rule('constant', B(lexer.space) * token(lexer.CONSTANT, word_match('null true false', true)))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', B(lexer.space) * token(lexer.COMMENT, lexer.to_eol('#')))

-- Timestamps.
local year = lexer.digit * lexer.digit * lexer.digit * lexer.digit
local month = lexer.digit * lexer.digit^-1
local day = lexer.digit * lexer.digit^-1
local date = year * '-' * month * '-' * day
local hours = lexer.digit * lexer.digit^-1
local minutes = lexer.digit * lexer.digit
local seconds = lexer.digit * lexer.digit
local fraction = '.' * lexer.digit^0
local time = hours * ':' * minutes * ':' * seconds * fraction^-1
local T = S(' \t')^1 + S('tT')
local zone = 'Z' + S(' \t')^0 * S('-+') * hours * (':' * minutes)^-1
lex:add_rule('timestamp', token('timestamp', date * (T * time * zone^-1)^-1))
lex:add_style('timestamp', lexer.styles.number)

-- Numbers.
local dec = lexer.digit^1 * ('_' * lexer.digit^1)^0
local hex = '0' * S('xX') * ('_' * lexer.xdigit^1)^1
local bin = '0' * S('bB') * S('01')^1 * ('_' * S('01')^1)^0
local integer = S('+-')^-1 * (hex + bin + dec)
local float = S('+-')^-1 *
  ((dec^-1 * '.' * dec + dec * '.' * dec^-1 * -P('.')) * (S('eE') * S('+-')^-1 * dec)^-1 +
    (dec * S('eE') * S('+-')^-1 * dec))
local special_num = S('+-')^-1 * '.' * word_match('inf nan', true)
lex:add_rule('number', B(lexer.space) * token(lexer.NUMBER, special_num + float + integer))

-- Types.
lex:add_rule('type', token(lexer.TYPE, '!!' * word_match({
  -- Collection types.
  'map', 'omap', 'pairs', 'set', 'seq',
  -- Scalar types.
  'binary', 'bool', 'float', 'int', 'merge', 'null', 'str', 'timestamp', 'value', 'yaml'
}, true) + '!' * lexer.range('<', '>', true)))

-- Document boundaries.
lex:add_rule('doc_bounds', token('document', lexer.starts_line(P('---') + '...')))
lex:add_style('document', lexer.styles.constant)

-- Directives
lex:add_rule('directive', token('directive', lexer.starts_line(lexer.to_eol('%'))))
lex:add_style('directive', lexer.styles.preprocessor)

-- Indicators.
local anchor = B(lexer.space) * token(lexer.LABEL, '&' * word)
local alias = token(lexer.VARIABLE, '*' * word)
local tag = token('tag', '!' * word * P('!')^-1)
local reserved = token(lexer.ERROR, S('@`') * word)
local indicator_chars = token(lexer.OPERATOR, S('-?:,>|[]{}!'))
lex:add_rule('indicator', tag + indicator_chars + alias + anchor + reserved)
lex:add_style('tag', lexer.styles.class)

return lex
