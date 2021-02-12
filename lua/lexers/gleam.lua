-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Gleam LPeg lexer
-- https://gleam.run/
-- Contributed by Tynan Beatty

local l = require("lexer")
local token, word_match = l.token, l.word_match
local B, P, R, S = lpeg.B, lpeg.P, lpeg.R, lpeg.S

local name = (l.lower + "_") * (l.lower + l.digit + "_")^0
local s = l.space

local M = {_NAME = "gleam"}

-- Whitespace
local whitespace_token = token(l.WHITESPACE, s^1)

-- Keywords
local keyword = word_match{
  "as", "assert", "case", "const", "external", "fn", "if", "import", "let",
  "opaque", "pub", "todo", "try", "tuple", "type",
}
local keyword_token = token(l.KEYWORD, keyword)

-- Types
local type_token = token(l.TYPE, l.upper * l.alnum^0)

-- Modules
--local import = B("import") * s^1 * name * (s^0 * P("/") * s^0 * name)^0
--local module_token = token("module", import + name * #(s^0 * P(".")))
local module_token = token("module", name * #(s^0 * P(".")))

-- Functions
local call = name * #(s^0 * P("("))
local pipe = B(".", "|>") * s^0 * name
local function_token = token(l.FUNCTION, call + pipe)

-- Identifiers
local discard_name = P("_") * name
local identifier_token =
  token("discard", discard_name) + token(l.IDENTIFIER, name)

-- Strings
local dq_str = l.delimited_range('"', false)
local string_token = token(l.STRING, dq_str)

-- Comments
local line_comment = "//" * l.nonnewline^0
local comment_token = token(l.COMMENT, line_comment + discard_name)

-- Numbers
local function can_neg(patt)
  return (B(s + S("+-/*%<>=&|:,.")) * P("-"))^-1 * patt
end
local function can_sep(patt)
  return ((P("_")^-1 * patt^1) + patt)^1
end
local dec = l.digit * can_sep(l.digit)^0
local float = dec * P(".") * dec^0
local bin = P("0") * S("bB") * can_sep(S("01"))
local oct = P("0") * S("oO") * can_sep(R("07"))
local hex = P("0") * S("xX") * can_sep(l.xdigit)
local number_token = token(l.NUMBER, can_neg(float) + bin + oct + hex + can_neg(dec))

-- Operators
local operator_token = token(l.OPERATOR, S("+-/*%<>!=&|#:,.()[]{}"))

M._rules = {
  {"whitespace", whitespace_token},
  {"keyword", keyword_token},
  {"type", type_token},
  {"module", module_token},
  {"function", function_token},
  {"identifier", identifier_token},
  {"string", string_token},
  {"comment", comment_token},
  {"number", number_token},
  {"operator", operator_token},
}

M._tokenstyles = {
  discard = l.STYLE_COMMENT,
  ["module"] = l.STYLE_CONSTANT,
}

M._foldsymbols = {
  _patterns = {"%l+", "[{}]", "[%[%]]", "[%(%)]", "//"},
  [l.COMMENT] = {["//"] = l.fold_line_comments("//")},
  [l.OPERATOR] = {["{"] = 1, ["}"] = -1, ["["] = 1, ["]"] = -1, ["("] = 1, [")"] = -1},
}

-- Gleam imports
local import = {_NAME = "gleam_import"}
import._rules = {
  {"whitespace", whitespace_token},
  {"keyword", token(l.KEYWORD, word_match{"as"})},
  {"module", token("module", name)},
  {"operator", operator_token},
}
local import_patt = P("import") * #(s^1 * name * (s^0 * P("/") * s^0 * name)^0)
local import_start_rule = token(l.KEYWORD, import_patt)
local import_end_rule = token("import", #(keyword - P("as")))
l.embed_lexer(M, import, import_start_rule, import_end_rule)

-- Gleam unqualified imports
local unqualified = {_NAME = "gleam_unqualified"}
unqualified._rules = {
  {"whitespace", whitespace_token},
  {"type", type_token},
  {"identifier", token(l.FUNCTION, name)},
  {"operator", operator_token},
}
l.embed_lexer(import, unqualified, token(l.OPERATOR, "."), token(l.OPERATOR, "}"))

return M
