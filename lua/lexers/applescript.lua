-- Copyright 2006-2022 Mitchell. See LICENSE.
-- Applescript LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('applescript')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'script', 'property', 'prop', 'end', 'copy', 'to', 'set', 'global', 'local', 'on', 'to', 'of',
  'in', 'given', 'with', 'without', 'return', 'continue', 'tell', 'if', 'then', 'else', 'repeat',
  'times', 'while', 'until', 'from', 'exit', 'try', 'error', 'considering', 'ignoring', 'timeout',
  'transaction', 'my', 'get', 'put', 'into', 'is',
  -- References.
  'each', 'some', 'every', 'whose', 'where', 'id', 'index', 'first', 'second', 'third', 'fourth',
  'fifth', 'sixth', 'seventh', 'eighth', 'ninth', 'tenth', 'last', 'front', 'back', 'st', 'nd',
  'rd', 'th', 'middle', 'named', 'through', 'thru', 'before', 'after', 'beginning', 'the',
  -- Commands.
  'close', 'copy', 'count', 'delete', 'duplicate', 'exists', 'launch', 'make', 'move', 'open',
  'print', 'quit', 'reopen', 'run', 'save', 'saving',
  -- Operators.
  'div', 'mod', 'and', 'not', 'or', 'as', 'contains', 'equal', 'equals', 'isn\'t'
}, true)))

-- Constants.
lex:add_rule('constant', token(lexer.CONSTANT, word_match({
  'case', 'diacriticals', 'expansion', 'hyphens', 'punctuation',
  -- Predefined variables.
  'it', 'me', 'version', 'pi', 'result', 'space', 'tab', 'anything',
  -- Text styles.
  'bold', 'condensed', 'expanded', 'hidden', 'italic', 'outline', 'plain', 'shadow',
  'strikethrough', 'subscript', 'superscript', 'underline',
  -- Save options.
  'ask', 'no', 'yes',
  -- Booleans.
  'false', 'true',
  -- Date and time.
  'weekday', 'monday', 'mon', 'tuesday', 'tue', 'wednesday', 'wed', 'thursday', 'thu', 'friday',
  'fri', 'saturday', 'sat', 'sunday', 'sun', 'month', 'january', 'jan', 'february', 'feb', 'march',
  'mar', 'april', 'apr', 'may', 'june', 'jun', 'july', 'jul', 'august', 'aug', 'september', 'sep',
  'october', 'oct', 'november', 'nov', 'december', 'dec', 'minutes', 'hours', 'days', 'weeks'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.alpha * (lexer.alnum + '_')^0))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range('"', true)))

-- Comments.
local line_comment = lexer.to_eol('--')
local block_comment = lexer.range('(*', '*)')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-^*/&<>=:,(){}')))

-- Fold points.
lex:add_fold_point(lexer.COMMENT, '(*', '*)')

return lex
