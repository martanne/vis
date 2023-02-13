## Unreleased

- add a changelog
- fix warning by dealing with error value from fchdir in text-io.c
- fix documentation of initial value to 'syntax' option
- fix a ~ being considered a special character in path patterns (except at the start)
- improvements to and clean-up of vis-open


## [0.8] - 2022-11-01

This is a release of vis as it has been for more than a year
before some development started up again. We're releasing this
version to get a stable 'old' release out there which should
still contain a number of bugfixes compared to 0.7.

- build: add git based version information back
- lexers: fix bug in bash lexer for last here-doc
- vis: make O implementation independent of mapping
- fix typos in comments
- lua: fix luacheck warnings
- vis: rename to/till motion internals
- vis: implement multiline to/till motions
- vis-lua: provide file.permission property
- Mention pkg-config in README
- lexers/strace: improve comments, field names and syscall
  results
- lexers/git-rebase: also highlight break command
- filetype: Set "bash" for APKBUILD and .ebuild.
- filetype: Detect make shebang for "makefile".
- Adding .sv extension to verilog syntax highlighter
- build: update alpine in docker build to version 3.13
- sam: only skip the last empty match if it follows a newline
- sam: produce empty match at the end of looped range
- test: update
- gitignore: remove vim specific swap files
- sam: tweak handling of zero length matches in y commands
- sam: simplify trailing match handling for x/y commands
- vis: correctly close pipe connected to stdin of external
  process
- add lua5.4 in configure script
- vis: Add readline Ctrl+A/E bindings
- ci: verify codecov script before using it
- ci: verify coverity scan script before using it
- filetype: Set "groovy" for Jenkinsfile


## [0.7.1] README: x/freenode/c/libera/ - 2022-05-03

- Update the README to point to irc.libera.chat after the great
  migration of 2021.


## [0.7] vis version 0.7 - 2020-12-08

This is mostly a bug fix release with
[fixes for a few cases of undefined behavior](https://www.brain-dump.org/blog/finding-undefined-behavior-in-c-code/)
and preliminary work for experimentation with different
[core text management data structures](https://www.brain-dump.org/blog/rethinking-vis-text-management-data-structure/)
and
[general editor architecture](https://www.brain-dump.org/blog/vis-to-server-or-not/).

- fix UB in core text management data structure
- text refactoring, splitting out reusable text iterator and I/O
  components
- new `*at()` variants taking directory descriptor for file
  load/save API
- more efficient initial file read, avoiding spurious syscalls
  and copy
- text API cleanups, const correctness improvements
- increased test coverage for core text data structure
- support for Lua 5.4
- Lua API improvements: `vis.mark`, `vis.register`,
  `vis.win.file.modified` and support for terminal CSI events
- NetBSD support
- new `:set ignorecase` option to search case independently
- new visual mode mapping `<C-a>` to select all matching
  selections
- fix mappings involving non-leading `<C-c>`
- minor file detection fixes for racket, node.js modules,
  Typescript and liliypond
- new lexers for Zig, meson build system, Mikrotik RouterOS
  scripts, Gemini
- improved inner word text object and its use for `<C-n>` in
  normal mode
- improved `<C-n>` behavior in visual mode
- removed `ie`, `ae` inner/outer entire text object, use `:`, as
  shorthand for `:0,$`.
- removed pariwise selection combinators `z>`, `z<`, `z-`, `z+`,
  `z&`, `z|`
- remove `~` as alias for `g~`
- use `~` instead of `!` for selection complement
- remove special key and window related aliases
- `vis-open(1)` adds a trailing slash to indicate folders
- add primary clipboard support to `vis-clipboard(1)`
- support wayland clipboard using `wl-clipboard(1)`
- new Makefile targets: `distclean`, `testclean`


## [0.6] vis version 0.6 - 2020-06-07

- bounded time syntax highlighting using the `:set redrawtime`
  option
- support optional count for sam's text commands e.g. `:i3/-/`
- make `<C-n>` in visual mode match next occurence of existing
  selection
- warn when attempting to write to an existing file
- improved file change detection based on inode instead of path
  information
- fix file saves with modifications in file pre-save events
- fix save on file systems without `fsync(2)` support on
  directory descriptors
- do not unlink `file~` when saving `file`
- introduce distinct `vis-menu(1)` exit codes
- modify Lua package.path to include <plugin>/init.lua
- performance improvements for the HTML, XML and YAML lexers
- new Julia and Elm lexers, better defaults for standard text
  lexer
- support optional exit status in `:q` and `:qall` commands
- better temporary file creation using `mkstemp(2)`
- performance improvements in highlight matching parentheses
- improved behavior of `^` and `$` in searches and looping
  commands
- improved search wrap around behavior
- new `:set layout` option to specify window orientation
- improved filetype detection by matching known filenames exactly
- support DragonFly BSD in configure script
- better manual page, fixed warnings
- removed `gp`, `gP`, `gq`
- implement `g~`, `gu` and `gU` using `tr(1)`, they are no longer
  operators
- removed `v` and `V` in operator pending mode
- avoid crash if `$TERM` is unset
- keep selections after `:>` command
- normalize selections after `:` command execution
- show pending input queue content in status bar
- make `r<Enter>` insert a new line
- new `:set loadmethod` option, valid values are `read`, `mmap`
  or `auto`
- always apply `:|` command to existing selections
- fix terminal UI on serial console
- various code cleanups, removal of VLA
- <Escape> resets count, if applicable
- fix `:X` and `:Y` commands which were interchanged
- don't strip executables by default, provide install-strip
  target


## [0.5] vis version 0.5 - 2018-03-25

- Fix for a buffer overflow when dealing with invalid/incomplete
  Unicode sequences which caused an infinite loop/crash. With
  default compilation flags this should not be exploitable, but
  opening a malicious file would lose all unsaved changes.
- Fix color support in ncurses 6.1
- New default 256 color theme: zenburn. It should hopefully work
  better with the default color palette.
- Updated Docker based builds (`make docker`) to use latest
  Alpine Linux packages to produce a statically linked,
  self-contained binary.
- Take symbolic keys into account when evaluating key prefixes
  (`ci<` is not a prefix of `ci<Tab>`).
- Improved paragraph text objects.
- Reset count after handling ,
- Lexer updates for Clojure, Scheme, ASM, Pony, PHP, Python,
  Erlang, xs and ReasonML.
- Correct handling of `g/^$/` construct to match empty lines in
  commands like:

    x g/^$/ d

- `<C-v><Enter>` now inserts `\r` rather than `\n`, this
  currently also affects `r<Enter>` which might not be desirable.
- Fix command prompt malfunction triggered by special cursor position.
- Configure script can be interrupted.
- Removed `!` operator, use `:|`
- Ignore `SIGQUIT`
- `vis-open(1)` fixes


## [0.4] vis version 0.4 - 2017-07-23

- Selections as core editing primitives. Cursors have been superseded by singleton selections. Overlapping selections are now merged. This change is also reflected in the exposed Lua API (for which still no stability guarantee is given).

- Selections can be saved into marks on which set operations can be performed:

  `m` save selections\
  `M` restore selections\
  `|` set union\
  `&` set intersection\
  `\` set minus\
  `!` set complement\
  `z|` pairwise union\
  `z&` pairwise intersection\
  `z+` pairwise combine, choose longer\
  `z-` pairwise combine, choose shorter\
  `z<` pairwise combine, choose leftmost\
  `z>` pairwise combine, choose rightmost

  Marks are specified using `'{mark}` analogous to `"{register}`.

- Jump list based on marks:

  `g<` jump backward\
  `g>` jump forward\
  `gs` save currently active selections

- New register `#` to insert the current selection number.
- Drop special handling of `\r\n` line endings. `\r` will be
  displayed as `^M`. Enter will always insert `\n`.
- Fix Unicode regex search with libtre backend.
- New count specifiers for sam's `g` and `v` commands to
  keep/drop selections based on their index.
- On macOS saving files larger than INT_MAX bytes should work.
- New `:set show-eof` to toggle the display of end of file markers ~
  as before it is enabled by default.
- Double leading slashes of paths are stripped.
- Improved `:<` command implementation to only use a pipe when
  necessary.
- New lexers for Myrddin and `strace(1)`, updates for Elixir, Perl
  and Forth.
- Fix compilation for GNU Hurd. The vis package is now built for
  all supported Debian architectures.
- Improve job control of forked processes. SIGINT is now properly
  delivered to child processes.
- Commands given a huge count can now be interrupted using `.`
- This is implemented in cooperative fashion, meaning a single
  long running operation can still not be interrupted.
- More efficient line wise motions based on optimized
  `mem{r,}chr(3)` libc functions.
- Optionally support vim compatible n/N search direction.
- Reproducible, statically linked, self contained binary built
  using the Alpine Docker image. The idea being that it is a
  single file which can be copied to any Linux >= 2.6 system to
  provide a usable editor. It has an embedded tar archive which
  contains the required Lua support files which are extracted to
  a temporary directory using libuntar.
- Preliminary C API documentation found at: http://vis.rtfd.io
- Updated manual page.
- Various code cleanups.

Check the git log for further details.

The release tarball is signed with [OpenBSD's signify
tool](http://man.openbsd.org/signify), the signature

    untrusted comment: verify with vis.pub
    RWRbDa94LCndL/v7m45zQw4saMKs5AsnTKBsvvFujZbAi9CIhlyiz0fihaWYbHkWDO957Csn5yJvecac+iUxX7arQ5IxZ4XRcQE=

can be verified using the following public key:

    untrusted comment: vis editor signify public key
    RWRbDa94LCndLy4pUdO6h1PmS1ooHOGb7p84OfQIR7+hFlZwuAXUdQ5J


## [0.3] vis version 0.3 - 2017-03-26

Most notable changes include:

- support for sam's structural regular expression based command language
- various bug fixes related to multiple cursor/selection support. New
  functionality to navigate among cursors (`<C-u>`, `<C-d>`), align
  (`<Tab>`, `<S-Tab>`), rotate (`+`, `-`), trim (`\`) or drop
  (`<C-c>`, `<C-l>`) selections.
- improved Lua API, featuring a new event subscription mechanism and the
  possibility to define custom operators, motions, text-objects, `:set`
  options and more. Notice however that at this point no API stability
  guarantee is provided.\
  You might have to update your `visrc.lua` configuration file, check the
  Documentation for details:
  http://martanne.github.io/vis/doc/
- new standalone tools vis-menu (`:o .`), vis-digraph (`<C-k>`) and
  vis-complete (`<C-k>`) for a simple file open dialog, digraph support
  and word completion, respectively.
- multiple bug fixes for vi(m) functionality, including improved count
  and repeat handling as well as better cursor positioning, `cw`, shift,
  join and autoindent implementation.
- new key mapping processing based on longest unique match
- optional libtre based regex backend for more memory efficient
  forward searches
- respect umask when creating new files, previously they were only
  read/writable by the current user. Also `fsync(2)` destination
  directory after `rename(2)` when performing an atomic save operation.
- new `:set` options to configure the used shell, escape
  delay, file save method and context to consider for syntax
  highlighting
- True color support in lexer themes, in case the terminal
  supports color palette changes
- minimal built-in `:help [pattern]` command
- incorporated upstream changes to LPeg based lexers used for syntax
  highlighting from the Scintillua project
- new set of manual pages in mdoc format
- experimental raw vt100 UI backend for resource constraint environments
- various code cleanups and bug fixes reported by static analysis, runtime
  interpretation and fuzzing tools

Check the git log for further details.

The release tarball is signed with [OpenBSD's signify
tool](http://man.openbsd.org/signify), the signature can be
verified using the following public key:

    untrusted comment: vis editor signify public key
    RWRbDa94LCndLy4pUdO6h1PmS1ooHOGb7p84OfQIR7+hFlZwuAXUdQ5J

## [0.2] vis version 0.2 - 2016-03-25

no changelog, 240 commits between v0.1 and v0.2 tags.

## [0.1] vis version 0.1 - 2015-12-31

