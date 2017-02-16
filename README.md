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
 * [TRE](http://laurikari.net/tre/) (optional for more memory efficient regex search)

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

End user documentation can be found in the [`vis(1)` manual page](http://martanne.github.io/vis/man/vis.1.html)
and the [Wiki](https://github.com/martanne/vis/wiki). Read the [FAQ](https://github.com/martanne/vis/wiki/FAQ)
for common questions.

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

How to Help?
============

There are plenty ways to contribute: writing core editor features using
C or extension in Lua, improving documentation or tests, packaging for
your favorite distribution, ...

Checkout the [Developer Overview](https://github.com/martanne/vis/wiki/Developer-Overview)
to get started and do not hesistate to ask question in the `#vis-editor`
IRC channel on freenode.
