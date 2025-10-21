# Vis - Combining Modal Editing with Structural Regular Expressions

[![Development discussion](https://img.shields.io/badge/email-~martanne%2Fdevel-black?logo=sourcehut)](https://lists.sr.ht/~martanne/devel)
[![builds.sr.ht status](https://builds.sr.ht/~martanne/vis/commits.svg)](https://builds.sr.ht/~martanne/vis/commits?)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/3939/badge.svg)](https://scan.coverity.com/projects/3939)
[![codecov](https://codecov.io/gh/martanne/vis/branch/master/graph/badge.svg)](https://codecov.io/gh/martanne/vis)
[![Documentation Status](https://readthedocs.org/projects/vis/badge/?version=master)](http://vis.readthedocs.io/en/master/?badge=master)
[![#vis-editor on libera](https://img.shields.io/badge/IRC-%23vis--editor-blue?logo=libera.chat)](ircs://irc.libera.chat:6697/vis-editor)

Vis aims to be a modern, legacy-free, simple yet efficient editor,
combining the strengths of both vi(m) and sam.

It extends vi's modal editing with built-in support for multiple
cursors/selections and combines it with [sam's](http://sam.cat-v.org/)
[structural regular expression](http://doc.cat-v.org/bell_labs/structural_regexps/)
based [command language](http://doc.cat-v.org/bell_labs/sam_lang_tutorial/).

A universal editor, it has decent Unicode support and should cope with arbitrary
files, including large, binary or single-line ones.

Efficient syntax highlighting is provided using
[Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar),
which can be conveniently expressed using [Lua](http://www.lua.org/)
in the form of [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/).

The editor core is written in a reasonable amount of clean (your mileage
may vary), modern and legacy-free C code, enabling it to run in
resource-constrained environments. The implementation should be easy to hack on
and encourages experimentation. There is also a Lua API for in-process
extensions.

Vis strives to be *simple* and focuses on its core task: efficient text
management. Clipboard and digraph handling as well as a fuzzy file open
dialog are all provided by independent utilities. There exist plans to use
a client/server architecture, delegating window management to your windowing
system or favorite terminal multiplexer.

The intention is *not* to be bug-for-bug compatible with vi(m). Instead,  
we aim to provide more powerful editing features based on an elegant design
and clean implementation.

[![vis demo](https://asciinema.org/a/41361.png)](https://asciinema.org/a/41361)

Build instructions
------------------

See [INSTALL.md](INSTALL.md) for detailed build instructions.

Or simply use one of the
[distribution provided packages](https://github.com/martanne/vis/wiki/Distribution-Packages).

Documentation
-------------

End user documentation can be found in the
[`vis(1)` manual page](http://martanne.github.io/vis/man/vis.1.html)
and the [Wiki](https://github.com/martanne/vis/wiki). Read the
[FAQ](https://github.com/martanne/vis/wiki/FAQ) for common questions.
Learn about some differences compared to
[`sam(1)`](https://github.com/martanne/vis/wiki/Differences-from-Sam) and
[`vim(1)`](https://github.com/martanne/vis/wiki/Differences-from-Vi(m)),
respectively.

[C API](https://vis.readthedocs.io/) as well as [Lua API](http://martanne.github.io/vis/doc/)
documentation is also available.

Non Goals
---------

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

How to help?
------------

There are plenty of ways to contribute, below are a few ideas:

 * Artwork
    - [Color Themes](https://github.com/martanne/vis/wiki/Themes)
    - [Name](https://github.com/martanne/vis/issues/338) +
      [Logo](https://github.com/martanne/vis/issues/328)
    - Homepage?
 * Documentation
    - [Manual Pages](https://github.com/martanne/vis/wiki/Developer-Overview#manual-pages)
    - Improve `:help` output
 * Lua
    - [API Documentation](https://github.com/martanne/vis/wiki/Developer-Overview#api-documentation)
      and Examples
    - [Unit Tests](https://github.com/martanne/vis-test/tree/master/lua)
    - [Plugin Development](https://github.com/martanne/vis/wiki/Plugins)
    - [API Design](https://github.com/martanne/vis/issues/292)
 * [Testing Infrastructure](https://github.com/martanne/vis-test)
 * [Distribution Packaging](https://github.com/martanne/vis/wiki/Distribution-Packages)
 * [Core Editor Design](https://github.com/martanne/vis/issues?q=is%3Aopen+is%3Aissue+label%3Adesign)

Checkout the [Developer Overview](https://github.com/martanne/vis/wiki/Developer-Overview)
to get started and do not hesitate to ask question in the `#vis-editor`
IRC channel on libera ([join via your browser](https://web.libera.chat/#vis-editor)).
