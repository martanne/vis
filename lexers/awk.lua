-- Copyright 2006-2015 Mitchell mitchell.att.foicica.com. See LICENSE.
-- AWK LPeg lexer.
-- Modified by Wolfgang Seeberg 2012, 2013.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'awk'}

local LEFTBRACKET = '['
local RIGHTBRACKET = ']'
local SLASH = '/'
local BACKSLASH = '\\'
local CARET = '^'
local CR = '\r'
local LF = '\n'
local CRLF = CR .. LF
local DQUOTE = '"'
local DELIMITER_MATCHES = {['('] = ')', ['['] = ']'}
local COMPANION = {['('] = '[', ['['] = '('}
local CC = {
  alnum = 1, alpha = 1, blank = 1, cntrl = 1, digit = 1, graph = 1, lower = 1,
  print = 1, punct = 1, space = 1, upper = 1, xdigit = 1
}
local LastRegexEnd = 0
local BackslashAtCommentEnd = 0
local KW_BEFORE_RX = {
  case = 1, ['do'] = 1, ['else'] = 1, exit = 1, print = 1, printf = 1,
  ['return'] = 1
}

local function findKeyword(input, e)
  local i = e
  while i > 0 and input:find("^[%l]", i) do i = i - 1 end
  local w = input:sub(i + 1, e)
  if i == 0 then
    return KW_BEFORE_RX[w] == 1
  elseif input:find("^[%u%d_]", i) then
    return false
  else
    return KW_BEFORE_RX[w] == 1
  end
end

local function isRegex(input, i)
  while i >= 1 and input:find('^[ \t]', i) do i = i - 1 end
  if i < 1 then return true end
  if input:find("^[-!%%&(*+,:;<=>?[^{|}~\f]", i) or findKeyword(input, i) then
    return true
  elseif input:sub(i, i) == SLASH then
    return i ~= LastRegexEnd -- deals with /xx/ / /yy/.
  elseif input:find('^[]%w)."]', i) then
    return false
  elseif input:sub(i, i) == LF then
    if i == 1 then return true end
    i = i - 1
    if input:sub(i, i) == CR then
      if i == 1 then return true end
      i = i - 1
    end
  elseif input:sub(i, i) == CR then
    if i == 1 then return true end
    i = i - 1
  else
    return false
  end
  if input:sub(i, i) == BACKSLASH and i ~= BackslashAtCommentEnd then
    return isRegex(input, i - 1)
  else
    return true
  end
end

local function eatCharacterClass(input, s, e)
  local i = s
  while i <= e do
    if input:find('^[\r\n]', i) then
      return false
    elseif input:sub(i, i + 1) == ':]' then
      local str = input:sub(s, i - 1)
      return CC[str] == 1 and i + 1
    end
    i = i + 1
  end
  return false
end

local function eatBrackets(input, i, e)
  if input:sub(i, i) == CARET then i = i + 1 end
  if input:sub(i, i) == RIGHTBRACKET then i = i + 1 end
  while i <= e do
    if input:find('^[\r\n]', i) then
      return false
    elseif input:sub(i, i) == RIGHTBRACKET then
      return i
    elseif input:sub(i, i + 1) == '[:' then
      i = eatCharacterClass(input, i + 2, e)
      if not i then return false end
    elseif input:sub(i, i) == BACKSLASH then
      i = i + 1
      if input:sub(i, i + 1) == CRLF then i = i + 1 end
    end
    i = i + 1
  end
  return false
end

local function eatRegex(input, i)
  local e = #input
  while i <= e do
    if input:find('^[\r\n]', i) then
      return false
    elseif input:sub(i, i) == SLASH then
      LastRegexEnd = i
      return i
    elseif input:sub(i, i) == LEFTBRACKET then
      i = eatBrackets(input, i + 1, e)
      if not i then return false end
    elseif input:sub(i, i) == BACKSLASH then
      i = i + 1
      if input:sub(i, i + 1) == CRLF then i = i + 1 end
    end
    i = i + 1
  end
  return false
end

local ScanRegexResult
local function scanGawkRegex(input, index)
  if isRegex(input, index - 2) then
    local i = eatRegex(input, index)
    if not i then
      ScanRegexResult = false
      return false
    end
    local rx = input:sub(index - 1, i)
    for bs in rx:gmatch("[^\\](\\+)[BSsWwy<>`']") do
      -- /\S/ is special, but /\\S/ is not.
      if #bs % 2 == 1 then return i + 1 end
    end
    ScanRegexResult = i + 1
  else
    ScanRegexResult = false
  end
  return false
end
-- Is only called immediately after scanGawkRegex().
local function scanRegex()
  return ScanRegexResult
end

local function scanString(input, index)
  local i = index
  local e = #input
  while i <= e do
    if input:find('^[\r\n]', i) then
      return false
    elseif input:sub(i, i) == DQUOTE then
      return i + 1
    elseif input:sub(i, i) == BACKSLASH then
      i = i + 1
      -- l.delimited_range() doesn't handle CRLF.
      if input:sub(i, i + 1) == CRLF then i = i + 1 end
    end
    i = i + 1
  end
  return false
end

-- purpose: prevent isRegex() from entering a comment line that ends with a
-- backslash.
local function scanComment(input, index)
  local _, i = input:find('[^\r\n]*', index)
  if input:sub(i, i) == BACKSLASH then BackslashAtCommentEnd = i end
  return i + 1
end

local function scanFieldDelimiters(input, index)
  local i = index
  local e = #input
  local left = input:sub(i - 1, i - 1)
  local count = 1
  local right = DELIMITER_MATCHES[left]
  local left2 = COMPANION[left]
  local count2 = 0
  local right2 = DELIMITER_MATCHES[left2]
  while i <= e do
    if input:find('^[#\r\n]', i) then
      return false
    elseif input:sub(i, i) == right then
      count = count - 1
      if count == 0 then return count2 == 0 and i + 1 end
    elseif input:sub(i, i) == left then
      count = count + 1
    elseif input:sub(i, i) == right2 then
      count2 = count2 - 1
      if count2 < 0 then return false end
    elseif input:sub(i, i) == left2 then
      count2 = count2 + 1
    elseif input:sub(i, i) == DQUOTE then
      i = scanString(input, i + 1)
      if not i then return false end
      i = i - 1
    elseif input:sub(i, i) == SLASH then
      if isRegex(input, i - 1) then
        i = eatRegex(input, i + 1)
        if not i then return false end
      end
    elseif input:sub(i, i) == BACKSLASH then
      if input:sub(i + 1, i + 2) == CRLF then
        i = i + 2
      elseif input:find('^[\r\n]', i + 1) then
        i = i + 1
      end
    end
    i = i + 1
  end
  return false
end

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '#' * P(scanComment))

-- Strings.
local string = token(l.STRING, DQUOTE * P(scanString))

-- Regular expressions.
-- Slash delimited regular expressions are preceded by most operators or
-- the keywords 'print' and 'case', possibly on a preceding line. They
-- can contain unescaped slashes and brackets in brackets. Some escape
-- sequences like '\S', '\s' have special meanings with Gawk. Tokens that
-- contain them are displayed differently.
local regex = token(l.REGEX, SLASH * P(scanRegex))
local gawkRegex = token('gawkRegex', SLASH * P(scanGawkRegex))

-- no leading sign because it might be binary.
local float = ((l.digit ^ 1 * ('.' * l.digit ^ 0) ^ -1) +
    ('.' * l.digit ^ 1)) * (S('eE') * S('+-') ^ -1 * l.digit ^ 1) ^ -1
-- Numbers.
local number = token(l.NUMBER, float)
local gawkNumber = token('gawkNumber', l.hex_num + l.oct_num)

-- Operators.
local operator = token(l.OPERATOR, S('!%&()*+,-/:;<=>?[\\]^{|}~'))
local gawkOperator = token('gawkOperator', P("|&") + "@" + "**=" + "**")

-- Fields. E.g. $1, $a, $(x), $a(x), $a[x], $"1", $$a, etc.
local field = token('field', P('$') * S('$+-') ^ 0 *
                    (float + (l.word ^ 0 * '(' * P(scanFieldDelimiters)) +
                     (l.word ^ 1 * ('[' * P(scanFieldDelimiters)) ^ -1) +
                     ('"' * P(scanString)) + ('/' * P(eatRegex) * '/')))

-- Functions.
local func = token(l.FUNCTION, l.word * #P('('))

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'BEGIN', 'END', 'atan2', 'break', 'close', 'continue', 'cos', 'delete', 'do',
  'else', 'exit', 'exp', 'fflush', 'for', 'function', 'getline', 'gsub', 'if',
  'in', 'index', 'int', 'length', 'log', 'match', 'next', 'nextfile', 'print',
  'printf', 'rand', 'return', 'sin', 'split', 'sprintf', 'sqrt', 'srand', 'sub',
  'substr', 'system', 'tolower', 'toupper', 'while'
})

local gawkKeyword = token('gawkKeyword', word_match{
  'BEGINFILE', 'ENDFILE', 'adump', 'and', 'asort', 'asorti', 'bindtextdomain',
  'case', 'compl', 'dcgettext', 'dcngettext', 'default', 'extension', 'func',
  'gensub', 'include', 'isarray', 'load', 'lshift', 'mktime', 'or', 'patsplit',
  'rshift', 'stopme', 'strftime', 'strtonum', 'switch', 'systime', 'xor'
})

local builtInVariable = token('builtInVariable', word_match{
  'ARGC', 'ARGV', 'CONVFMT', 'ENVIRON', 'FILENAME', 'FNR', 'FS', 'NF', 'NR',
  'OFMT', 'OFS', 'ORS', 'RLENGTH', 'RS', 'RSTART', 'SUBSEP'
})

local gawkBuiltInVariable = token('gawkBuiltInVariable', word_match {
  'ARGIND', 'BINMODE', 'ERRNO', 'FIELDWIDTHS', 'FPAT', 'FUNCTAB', 'IGNORECASE',
  'LINT', 'PREC', 'PROCINFO', 'ROUNDMODE', 'RT', 'SYMTAB', 'TEXTDOMAIN'
})

-- Within each group order matters, but the groups themselves (except the
-- last) can be in any order.
M._rules = {
  {'whitespace', ws},

  {'comment', comment},

  {'string', string},

  {'field', field},

  {'gawkRegex', gawkRegex},
  {'regex', regex},
  {'gawkOperator', gawkOperator},
  {'operator', operator},

  {'gawkNumber', gawkNumber},
  {'number', number},

  {'keyword', keyword},
  {'builtInVariable', builtInVariable},
  {'gawkKeyword', gawkKeyword},
  {'gawkBuiltInVariable', gawkBuiltInVariable},
  {'function', func},
  {'identifier', identifier},
}

M._tokenstyles = {
  builtInVariable = l.STYLE_CONSTANT,
  default = l.STYLE_ERROR,
  field = l.STYLE_LABEL,
  gawkBuiltInVariable = l.STYLE_CONSTANT..',underlined',
  gawkKeyword = l.STYLE_KEYWORD..',underlined',
  gawkNumber = l.STYLE_NUMBER..',underlined',
  gawkOperator = l.STYLE_OPERATOR..',underlined',
  gawkRegex = l.STYLE_PREPROCESSOR..',underlined',
  regex = l.STYLE_PREPROCESSOR
}

M._foldsymbols = {
  _patterns = {'[{}]', '#'},
  [l.OPERATOR] = {['{'] = 1, ['}'] = -1},
  [l.COMMENT] = {['#'] = l.fold_line_comments('#')}
}

return M
