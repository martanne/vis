Vis a vim-like text editor
==========================

[![Linux Build Status](https://travis-ci.org/martanne/vis.svg?branch=master)](https://travis-ci.org/martanne/vis)
[![Cygwin Build Status](https://ci.appveyor.com/api/projects/status/61059w4jpdnb77ft/branch/master?svg=true)](https://ci.appveyor.com/project/martanne/vis/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/3939/badge.svg)](https://scan.coverity.com/projects/3939)
[![codecov](https://codecov.io/gh/martanne/vis/branch/master/graph/badge.svg)](https://codecov.io/gh/martanne/vis)
[![#vis-editor on freenode](https://img.shields.io/badge/IRC-%23vis--editor-blue.svg)](irc://irc.freenode.net/vis-editor)

Vis aims to be a modern, legacy free, simple yet efficient editor
combining the strengths of both vi(m) and sam.

It extends vi's modal editing with built-in support for multiple
cursors/selections and combines it with [sam's](http://sam.cat-v.org/)
[structural regular expression](http://doc.cat-v.org/bell_labs/structural_regexps/)
based [command language](http://doc.cat-v.org/bell_labs/sam_lang_tutorial/).

As an universal editor it has decent Unicode support (including double width
and combining characters) and should cope with arbitrary files including:

 - large (up to a few Gigabytes) ones including
   - Wikipedia/OpenStreetMap XML / SQL / CSV dumps
   - amalgamated source trees (e.g. SQLite)
 - single line ones e.g. minified JavaScript
 - binary ones e.g. ELF files

Efficient syntax highlighting is provided using
[Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar)
which can be conveniently expressed using [Lua](http://www.lua.org/)
in the form of [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/).

The editor core is written in a reasonable amount of clean (your mileage
may vary), modern and legacy free C code enabling it to run in resource
constrained environments. The implementation should be easy to hack on
and encourage experimentation. There also exists a Lua API for in process
extensions.

Vis strives to be *simple* and focuses on its core task: efficient text
management. As an example the file open dialog is provided by an independent
utility. There exist plans to use a client/server architecture, delegating
window management to your windowing system or favorite terminal multiplexer.

The intention is *not* to be bug for bug compatible with vi(m), instead 
we aim to provide more powerful editing features based on an elegant design
and clean implementation.

[![vis demo](https://asciinema.org/a/41361.png)](https://asciinema.org/a/41361)

Getting started / Build instructions
====================================

In order to build vis you will need a
[C99](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)
compiler, a [POSIX.1-2008](http://pubs.opengroup.org/onlinepubs/9699919799/)
compatible environment as well as:

 * [libcurses](http://www.gnu.org/software/ncurses/), preferably in the
   wide-character version
 * [libtermkey](http://www.leonerd.org.uk/code/libtermkey/)
 * [Lua](http://www.lua.org/) >= 5.2 (optional)
 * [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) >= 0.12
   (optional runtime dependency required for syntax highlighting)

Assuming these dependencies are met, execute:

    $ ./configure && make && sudo make install

By default the `configure` script will try to auto detect support for
Lua. See `configure --help` for a list of supported options. You can
also manually tweak the generated `config.mk` file.

Or simply use one of the distribution provided packages:

 * [ArchLinux](http://www.archlinux.org/packages/?q=vis)
 * [Alpine Linux](https://pkgs.alpinelinux.org/packages?name=vis)
 * [NixOS](https://github.com/NixOS/nixpkgs/blob/master/pkgs/applications/editors/vis/default.nix)
 * [Source Mage GNU/Linux](http://download.sourcemage.org/grimoire/codex/test/editors/vis)
 * [Void Linux](https://github.com/voidlinux/void-packages/tree/master/srcpkgs/vis)
 * [pkgsrc](http://pkgsrc.se/wip/vis-editor)

Documentation
=============

End user documentation can be found in the [`vis(1)` manual page](http://martanne.github.io/vis/man/vis.1.html).

[Lua API Documentation](http://martanne.github.io/vis/doc/) is also available.

Non Goals
=========

  Some features which will *not* be implemented:

   - tabs / multiple workspaces / advanced window management
   - file and directory browser
   - support for file archives (tar, zip, ...)
   - support for network protocols (ftp, http, ssh ...)
   - encryption
   - compression
   - GUIs (neither x11, motif, gtk, win32 ...) although the codebase
     should make it easy to add them
   - VimL
   - right-to-left text
   - ex mode, we have more elegant structural regexp
   - diff mode
   - vimgrep
   - internal spell checker
   - lots of compile time configurable features / `#ifdef` mess

Future Plans / Ideas
====================

This section contains some ideas for further architectural changes.

Event loop with asynchronous I/O
--------------------------------

The editor core should feature a proper main loop mechanism supporting
asynchronous non-blocking and always cancelable tasks which could be
used for all possibly long lived actions. Ideally the editor core would
never block and always remain responsive.

Client/Server Architecture / RPC interface
------------------------------------------

In principle it would be nice to follow a similar client/server approach
as [sam/samterm](http://sam.cat-v.org/) i.e. having the main editor as a
server and each window as a separate client process with communication
over a unix domain socket.

That way window management would be taken care of by dwm or dvtm and the
different client processes would still share common cut/paste registers
etc.

This would also enable a language agnostic plugin system.

Efficient Search and Replace
----------------------------

Currently the editor copies the whole text to a contiguous memory block
and then uses the standard regex functions from libc. Clearly this is not
a satisfactory solution for large files.

The long term solution is to write our own regular expression engine or
modify an existing one to make use of the iterator API. This would allow
efficient search without having to double memory consumption.

The used regex engine should use a non-backtracking algorithm. Useful
resources include:

 - [Russ Cox's regex page](http://swtch.com/~rsc/regexp/)
 - [TRE](https://github.com/laurikari/tre) as
   [used by musl](http://git.musl-libc.org/cgit/musl/tree/src/regex)
   which uses a parallel [TNFA matcher](http://laurikari.net/ville/spire2000-tnfa.ps)
 - [Plan9's regex library](http://plan9.bell-labs.com/sources/plan9/sys/src/libregexp/)
   which has its root in Rob Pike's sam text editor
 - [RE2](https://github.com/google/re2) C++ regex library

Developer Overview
==================

Feel free to join `#vis-editor` on freenode to discuss development related issues.

See [piece-table.md](piece-table.md) for an explanation of the core data structure.


A quick overview over the code structure to get you started:

 File(s)             | Description
 ------------------- | -----------------------------------------------------
 `array.[ch]`        | dynamically growing array, can store arbitrarily sized objects
 `buffer.[ch]`       | dynamically growing buffer used for registers and macros
 `config.def.h`      | definition of default key bindings (mapping of key actions)
 `lexers/`           | Lua LPeg based lexers used for syntax highlighting
 `main.c`            | key action definitions, program entry point
 `map.[ch]`          | crit-bit tree based map supporting unique prefix lookups and ordered iteration, used to implement `:`-commands and run time key bindings
 `register.[ch]`     | register implementation, system clipboard integration via `vis-clipboard`
 `ring-buffer.[ch]`  | fixed size ring buffer used for the jump list
 `sam.[ch]`          | structural regular expression based command language
 `text.[ch]`         | low level text / marks / {un,re}do tree / piece table implementation
 `text-motions.[ch]` | movement functions take a file position and return a new one
 `text-objects.[ch]` | functions take a file position and return a file range
 `text-regex.[ch]`   | text search functionality, designated place for regex engine
 `text-util.[ch]`    | text related utility functions mostly dealing with file ranges
 `ui-curses.[ch]`    | a terminal / curses based user interface implementation
 `ui.h`              | abstract interface which has to be implemented by ui backends
 `view.[ch]`         | ui-independent viewport, shows part of a file, syntax highlighting, cursor placement, selection handling
 `vis-cmds.c`        | vi(m) `:`-command implementation
 `vis-core.h`        | internal header file, various structs for core editor primitives
 `vis.c`             | vi(m) specific editor frontend implementation
 `vis.h`             | vi(m) specific editor frontend library public API
 `vis-lua.[ch]`      | Lua bindings, exposing core vis APIs for in process extension
 `vis-modes.c`       | vi(m) mode switching, enter/leave event handling
 `vis-motions.c`     | vi(m) cursor motion implementations, uses `text-motions.h` internally
 `vis-operators.c`   | vi(m) operator implementation
 `vis-prompt.c`      | `:`, `/` and `?` prompt implemented as a regular file/window with custom key bindings
 `vis-text-objects.c`| vi(m) text object implementations, uses `text-objects.h` internally
 `vis.lua`           | Lua library for vis, providing parts of the exposed API
 `visrc.lua`         | Lua startup and configuration script

Testing infrastructure for the [low level core data structures]
(https://github.com/martanne/vis-test/tree/master/core), [vim compatibility]
(https://github.com/martanne/vis-test/tree/master/vim), [sam compatibility]
(https://github.com/martanne/vis-test/tree/master/sam), [vis specific features]
(https://github.com/martanne/vis-test/tree/master/vis) and the [Lua API]
(https://github.com/martanne/vis-test/tree/master/lua) is in place, but
lacks proper test cases.
