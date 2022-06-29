Lua LPeg lexers for vis
=======================

Vis reuses the [Lua](http://www.lua.org/) [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/)
based lexers from the [Scintillua](https://orbitalquark.github.io/scintillua/index.html) project.

# Vis integration

Vis searches the lexers in the following locations (check the end of the
`:help` output for the exact paths used by your binary):

 * `$VIS_PATH/lexers`
 * `./lua/lexers` relative to the binary location (using `/proc/self/exe`)
 * `$XDG_CONFIG_HOME/vis/lexers` where `$XDG_CONFIG_HOME` refers to
   `$HOME/.config` if unset.
 * `/etc/vis/lexers`
 * `/usr/local/share/vis/lexers` or `/usr/share/vis/lexers` depending on
    the build configuration
 * `package.path` the standard Lua search path is queried for `lexers/$name`

At runtime a specific lexer can be loded by means of `:set syntax <name>`
where `<name>` corresponds to the filename without the `.lua` extension.

# Adding new lexers

To add a new lexer, start with the template quoted below or a lexer of a
similiar language. Read the
[lexer module documentation](https://orbitalquark.github.io/scintillua/api.html#lexer).
The [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) introduction might also
be useful.

For development purposes it is recommended to test the lexers from a lua
script as described in the
[Scintillua manual](https://orbitalquark.github.io/scintillua/manual.html#Using.Scintillua.as.a.Lua.Library).

To enable auto syntax highlighting when opening a file you can associate your
new lexer with a set of file extensions by adding a corresponding entry into
the table found in [`plugins/filetype.lua`](../plugins/filetype.lua) file.

Changes to existing lexers should also be sent upstream for consideration.

A template for new lexers:

```lua
-- Copyright 2006-2021 Mitchell. See LICENSE.
-- ? LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('?')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match[[
  keyword1 keyword2 keyword3
]]))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-*/%^=<>,.{}[]()')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, 'start', 'end')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))

return lex
```

# Color Themes

The [`../themes directory`](../themes) contains the color
schemes. Depending on the number of colors supported by your terminal,
vis will start with either the [`default-16`](../themes/default-16.lua)
or [`default-256`](../themes/default-256.lua) theme. Symlink it to
your prefered style or add a command like the following one to your
`visrc.lua`:

```
vis:command("set theme solarized")
```

# Dependencies

 * [Lua](http://www.lua.org/) 5.1 or greater
 * [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) 1.0.0 or greater
