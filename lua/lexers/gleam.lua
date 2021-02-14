-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- Gleam LPeg lexer
-- https://gleam.run/
-- Contributed by Tynan Beatty

local l = require("lexer")
local token, word_match = l.token, l.word_match
local B, P, R, S = lpeg.B, lpeg.P, lpeg.R, lpeg.S

local ID, KEY, OP = l.IDENTIFIER, l.KEYWORD, l.OPERATOR
local name = (l.lower + "_") * (l.lower + l.digit + "_")^0

local fn_name = token(l.FUNCTION, name)
local id = token(ID, name)
local mod_name = token("module", name)

local M = {_NAME = "gleam"}

-- Whitespace
local s = token(l.WHITESPACE, l.space^1)

-- Types
local typ_tok = token(l.TYPE, l.upper * l.alnum^0)

-- Modules
local typ_or_fn = (typ_tok + fn_name)
local mod_tok =
  token(KEY, "import") * s^1 * mod_name
  * (s^0 * token(OP, "/") * s^0 * mod_name)^0
  * (s^1 * token(KEY, "as") * s^1 * mod_name)^-1
  * (
    s^0 * token(OP, ".") * s^0 * token(OP, "{")
    * s^0 * typ_or_fn
    * (s^0 * token(OP, ",") * s^0 * typ_or_fn)^0
    * s^0 * token(OP, "}")
  )^-1

-- Keywords
local keyword = word_match{
  "as", "assert", "case", "const", "external", "fn", "if", "import", "let",
  "opaque", "pub", "todo", "try", "tuple", "type",
}
local key_tok = token(KEY, keyword)

-- Identifiers
local discard_id = token("discard", "_" * name)
local id_tok = discard_id + id

-- Functions
local mod_name_op = mod_name * s^0 * token(OP, ".")
local fn_def_call = mod_name_op^-1 * s^0 * fn_name * s^0 * #P("(")
local fn_pipe = token(OP, "|>") * s^0 * (token(KEY, "fn") + mod_name_op^-1 * fn_name)
local fn_tok = fn_def_call + fn_pipe

-- Strings
local str_tok = token(l.STRING, l.delimited_range('"', false))

-- Comments
local com_tok = token(l.COMMENT, "//" * l.nonnewline^0)

-- Labels
local lab_tok = token(OP, S("(,")) * s^0 * token(l.LABEL, name) * #(s^1 * id)

-- Numbers
local function can_neg(patt)
  return (B(l.space + S("+-/*%<>=&|:,.")) * "-")^-1 * patt
end
local function can_sep(patt)
  return (P("_")^-1 * patt^1)^1
end
local dec = l.digit * can_sep(l.digit)^0
local float = dec * "." * dec^0
local bin = "0" * S("bB") * can_sep(S("01"))
local oct = "0" * S("oO") * can_sep(R("07"))
local hex = "0" * S("xX") * can_sep(l.xdigit)
local num_tok = token(l.NUMBER, can_neg(float) + bin + oct + hex + can_neg(dec))

-- Operators
local op_tok = token(OP, S("+-*/%#!=<>&|.,:;{}[]()"))

-- Errors
local err_tok = token(l.ERROR, l.any)

M._rules = {
  {"whitespace", s},
  {"type", typ_tok},
  {"module", mod_tok},
  {"keyword", key_tok},
  {"function", fn_tok},
  {"label", lab_tok},
  {"identifier", id_tok},
  {"string", str_tok},
  {"comment", com_tok},
  {"number", num_tok},
  {"operator", op_tok},
  {"errors", err_tok},
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

-- Bit Strings
local bitstring = {_NAME = "gleam_bitstring"}
local bstr_key_tok =
  key_tok
  + token(KEY, word_match{
    "binary", "bytes", "int", "float", "bit_string", "bits",
    "utf8", "utf16", "utf32", "utf8_codepoint", "utf16_codepoint", "utf32_codepoint",
    "signed", "unsigned", "big", "little", "native", "unit", "size",
  })
bitstring._rules = {
  {"whitespace", s},
  {"type", typ_tok},
  {"module", mod_tok},
  {"keyword", bstr_key_tok},
  {"function", fn_tok},
  {"label", lab_tok},
  {"identifier", id_tok},
  {"string", str_tok},
  {"comment", com_tok},
  {"number", num_tok},
  {"operator", op_tok},
  {"errors", err_tok},
}
local bitstring_begin = token(OP, "<<")
local bitstring_end = token(OP, ">>")
l.embed_lexer(M, bitstring, bitstring_begin, bitstring_end)

return M
