-- Copyright 2022-2024 Mitchell. See LICENSE.
-- LPeg lexer for tool output.
-- If a warning or error is recognized, tags its filename, line, column (if available),
-- and message, and sets the line state to 1 for an error (first bit), and 2 for a warning
-- (second bit).
-- This is similar to Lexilla's errorlist lexer.

local lexer = lexer
local starts_line = lexer.starts_line
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(..., {lex_by_line = true})

-- Tags a pattern as plain text.
local function text(patt) return lex:tag(lexer.DEFAULT, patt) end

-- Tags a pattern as a filename.
local function filename(patt) return lex:tag('filename', patt) end

-- Typical line and column number patterns.
local line = text('line ')^-1 * lex:tag('line', lexer.dec_num)
local column = lex:tag('column', lexer.dec_num)

-- Tags a pattern as an error/warning/etc. message.
local function message(patt) return lex:tag('message', patt) end

-- Immediately marks the current line as an error.
-- This should only be specified at the end of a rule, or else LPeg may backtrack and mistakenly
-- mark a non-error line.
local function mark_error(_, pos)
  lexer.line_state[lexer.line_from_position(pos)] = 1
  return true
end

-- Immediately marks the current line as a warning.
-- This should only be specified at the end of a rule, or else LPeg may backtrack and mistakenly
-- mark a non-warning line.
local function mark_warning(_, pos)
  lexer.line_state[lexer.line_from_position(pos)] = 2
  return true
end

-- filename:line: message (ruby)
-- filename:line:col: message (c, cpp, go, ...)
-- filename: line X: message (bash)
local c_filename = filename((lexer.nonnewline - ':')^1)
local colon = text(':' * P(' ')^-1)
local warning = message(lexer.to_eol('warning: ')) * mark_warning
local note = message(lexer.to_eol('note: ')) -- do not mark
local error = message(lexer.to_eol()) * mark_error
lex:add_rule('common', starts_line(c_filename) * colon * line * colon * (column * colon)^-1 *
  (warning + note + error))

-- prog: filename:line: message (awk, lua)
lex:add_rule('prog', starts_line(text(lexer.word)) * colon * c_filename * colon * line * colon *
  (warning + error))

-- File "filename", line X (python)
local py_filename = filename((lexer.nonnewline - '"')^1)
lex:add_rule('python',
  starts_line(text('File "'), true) * py_filename * text('", ') * line * mark_error)

-- filename(line): error: message (d, cuda)
local lparen, rparen = text('('), text(')')
local d_filename = filename((lexer.nonnewline - '(')^1)
local d_error = message(lexer.to_eol(S('Ee') * 'rror')) * mark_error
lex:add_rule('dmd', starts_line(d_filename) * lparen * line * rparen * colon * d_error)

-- "filename" line X: message (gnuplot)
local gp_filename = filename((lexer.nonnewline - '"')^1)
lex:add_rule('gnuplot', starts_line(text('"')) * gp_filename * text('" ') * line * colon * error)

-- at com.path(filename:line) (java)
lex:add_rule('java',
  starts_line(text('at ' * (lexer.nonnewline - '(')^1), true) * lparen * c_filename * colon * line *
    rparen * mark_error)

-- message in filename on line X (php)
lex:add_rule('php', starts_line(message((lexer.nonnewline - ' in ')^1)) * text(' in ') *
  filename((lexer.nonnewline - ' on ')^1) * text(' on ') * line * mark_error)

-- filename(line, col): message (vb, csharp, fsharp, ...)
lex:add_rule('vb',
  starts_line(filename((lexer.nonnewline - '(')^1)) * lparen * line * text(', ') * column * rparen *
    colon * error)

-- message at filename line X (perl)
lex:add_rule('perl', starts_line(message((lexer.nonnewline - ' at ')^1)) * text(' at ') *
  filename((lexer.nonnewline - ' line ')^1) * text(' line ') * line * mark_error)

-- CMake Error at filename:line: (cmake)
lex:add_rule('cmake',
  starts_line(text('CMake Error at ')) * c_filename * colon * line * colon * mark_error)

lex:add_rule('any_line', lex:tag(lexer.DEFAULT, lexer.to_eol()))

return lex
