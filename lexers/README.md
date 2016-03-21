Lua LPeg lexers for vis
=======================

Vis reuses the [Lua](http://www.lua.org/) [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/)
based lexers from the [Scintillua](http://foicica.com/scintillua/) project.

# Vis integration

Vis searches the lexers in the following locations:

 * `$VIS_PATH/lexers`
 * `./lexers` relative to the binary location (using `/proc/self/exe`)
 * `$XDG_CONFIG_HOME/vis/lexers`
 * `/usr/local/share/vis/lexers`
 * `/usr/share/vis/lexers`
 * `package.path` (standard lua search path)

at runtime a specific lexer can be loded by means of `:set syntax <name>`
where `<name>` corresponds to the filename without the `.lua` extension.

# Adding new lexers

To add a new lexer, start with the `template.txt` found in this directory
or a lexer of a similiar language. Read the 
[lexer module documentation](http://foicica.com/scintillua/api.html#lexer).
The [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) introduction might also
be useful.

For development purposes it is recommended to test the lexers from a lua
script as described in the
[Scintillua manual](http://foicica.com/scintillua/manual.html#Using.Scintillua.as.a.Lua.Library).

To enable auto syntax highlighting when opening a file you can associate your
new lexer with a set of file extensions by adding a corresponding entry into
the table found at the end of the [lexer.lua](lexer.lua) file.

Changes to existing lexers should also be sent upstream for consideration.

# Color Themes

The `themes` sub directory contains the color schemes. At startup the
`default.lua` theme which should be a symlink to your prefered style is
used. Themes can be changed at runtime via the `:set theme <name>`
command where `<name>` does not include the `.lua` file extension.

# Dependencies

 * [Lua](http://www.lua.org/) 5.1 or greater
 * [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) 0.12 or greater
