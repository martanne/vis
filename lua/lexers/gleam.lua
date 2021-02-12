-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Gleam LPeg lexer
-- https://gleam.run/
-- Contributed by Tynan Beatty

local l = require("lexer")
local token, word_match = l.token, l.word_match
local B, P, R, S = lpeg.B, lpeg.P, lpeg.R, lpeg.S

local FN, KEY, OP = l.FUNCTION, l.KEYWORD, l.OPERATOR
local name = (l.lower + "_") * (l.lower + l.digit + "_")^0

local M = {_NAME = "gleam"}

-- Whitespace
local s = token(l.WHITESPACE, l.space^1)

-- Types
local typ_tok = token(l.TYPE, l.upper * l.alnum^0)

-- Modules
local mod_name = token("module", name)
local mod_name_tok = mod_name * s^0 * token(OP, ".")
local typ_or_fn = (typ_tok + token(FN, name))
local mod_tok =
  mod_name_tok
  + token(KEY, "import") * s^1 * mod_name
    * (s^0 * token(OP, "/") * s^0 * mod_name)^0
    * (s^1 * token(KEY, "as") * s^1 * mod_name)^-1
    * (
      s^0 * token(OP, ".") * s^0 * token(OP, "{")
      * s^0 * typ_or_fn
      * (s^0 * token(OP, ",") * s^0 * typ_or_fn)^0
      * s^0 * token(OP, "}")
    )^-1

-- Functions
local fn_tok =
  token(FN, name * #(l.space^0 * P("(")))
  + token(OP, "|>") * s^0
    * (token(KEY, "fn") + mod_name_tok^-1 * token(FN, name))

-- Keywords
local keyword = word_match{
  "as", "assert", "case", "const", "external", "fn", "if", "import", "let", "opaque",
  "pub", "todo", "try", "tuple", "type",
}
local key_tok = token(KEY, keyword)

-- Identifiers
local discard_name = P("_") * name
local id_tok = token("discard", discard_name) + token(l.IDENTIFIER, name)

-- Strings
local str_tok = token(l.STRING, l.delimited_range('"', false))

-- Comments
local line_comment = "//" * l.nonnewline^0
local com_tok = token(l.COMMENT, line_comment + discard_name)

-- Numbers
local function can_neg(patt)
  return (B(l.space + S("+-/*%<>=&|:,.")) * P("-"))^-1 * patt
end
local function can_sep(patt)
  return ((P("_")^-1 * patt^1) + patt)^1
end
local dec = l.digit * can_sep(l.digit)^0
local float = dec * P(".") * dec^0
local bin = P("0") * S("bB") * can_sep(S("01"))
local oct = P("0") * S("oO") * can_sep(R("07"))
local hex = P("0") * S("xX") * can_sep(l.xdigit)
local num_tok = token(l.NUMBER, can_neg(float) + bin + oct + hex + can_neg(dec))

-- Operators
local op_tok = token(OP, S("+-/*%<>!=&|#:,.()[]{}"))

-- Errors
local err_tok = token(l.ERROR, l.any)

M._rules = {
  {"whitespace", s},
  {"type", typ_tok},
  {"module", mod_tok},
  {"function", fn_tok},
  {"keyword", key_tok},
  {"identifier", id_tok},
  {"string", str_tok},
  {"comment", com_tok},
  {"number", num_tok},
  {"operator", op_tok},
}

M._tokenstyles = {
  discard = l.STYLE_COMMENT,
  ["module"] = l.STYLE_CONSTANT,
}

M._foldsymbols = {
  _patterns = {"%l+", "[{}]", "[%[%]]", "[%(%)]", "//"},
  [l.COMMENT] = {["//"] = l.fold_line_comments("//")},
  [OP] = {["{"] = 1, ["}"] = -1, ["["] = 1, ["]"] = -1, ["("] = 1, [")"] = -1},
}

-- Imports
--local import = {_NAME = "gleam_import"}
--import._rules = {
--  {"whitespace", s},
--  {"keyword", token(KEY, word_match{"as"})},
--  {"module", mod_name},
--  {"operator", op_tok},
--  {"error", err_tok},
--}
--local import_begin = token(KEY, P("import"))
--local import_end = token("import", #(keyword - P("as")))
--l.embed_lexer(M, import, import_begin, import_end)

-- Unqualified imports
--local unqual = {_NAME = "gleam_unqual"}
--unqual._rules = {
--  {"whitespace", s},
--  {"type", typ_tok},
--  {"identifier", token(FN, name)},
--  {"operator", op_tok},
--  {"error", err_tok},
--}
--local unqual_begin = token(OP, ".")
--local unqual_end = token(OP, "}")
--l.embed_lexer(import, unqual, unqual_begin, unqual_end)

return M
