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
 * [sregex](https://github.com/openresty/sregex/) >= v0.0.1rc1 (optional)

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

Text management using a piece table/chain
=========================================

The core of this editor is a persistent data structure called a piece
table which supports all modifications in `O(m)`, where `m` is the number
of non-consecutive editing operations. This bound could be further
improved to `O(log m)` by use of a balanced search tree, however the
additional complexity doesn't seem to be worth it, for now.

The actual data is stored in buffers which are strictly append only.
There exist two types of buffers, one fixed-sized holding the original
file content and multiple append-only ones storing the modifications.

A text, i.e. a sequence of bytes, is represented as a double linked
list of pieces each with a pointer into a buffer and an associated
length. Pieces are never deleted but instead always kept around for
redo/undo support. A span is a range of pieces, consisting of a start
and end piece. Changes to the text are always performed by swapping
out an existing, possibly empty, span with a new one.

An empty document is represented by two special sentinel pieces which
always exist:

    /-+ --> +-\
    | |     | |
    \-+ <-- +-/
     #1     #2

Loading a file from disk is as simple as mmap(2)-ing it into a buffer,
creating a corresponding piece and adding it to the double linked list.
Hence loading a file is a constant time operation i.e. independent of
the actual file size (assuming the operating system uses demand paging).

    /-+ --> +-----------------+ --> +-\
    | |     | I am an editor! |     | |
    \-+ <-- +-----------------+ <-- +-/
     #1             #3              #2

Insert
------

Inserting a junk of data amounts to appending the new content to a
modification buffer. Followed by the creation of new pieces. An insertion
in the middle of an existing piece requires the creation of 3 new pieces.
Two of them hold references to the text before respectively after the
insertion point. While the third one points to the newly added text.

    /-+ --> +---------------+ --> +----------------+ --> +--+ --> +-\
    | |     | I am an editor|     |which sucks less|     |! |     | |
    \-+ <-- +---------------+ <-- +----------------+ <-- +--+ <-- +-/
     #1            #4                   #5                #6      #2

           modification buffer content: "which sucks less"

During this insertion operation the old span [3,3] has been replaced
by the new span [4,6]. Notice that the pieces in the old span were not
changed, therefore still point to their predecessors/successors, and can
thus be swapped back in.

If the insertion point happens to be at a piece boundary, the old span
is empty, and the new span only consists of the newly allocated piece.

Delete
------

Similarly a delete operation splits the pieces at appropriate places.

    /-+ --> +-----+ --> +--+ --> +-\
    | |     | I am|     |! |     | |
    \-+ <-- +-----+ <-- +--+ <-- +-/
     #1       #7         #6      #2

Where the old span [4,5] got replaced by the new span [7,7]. The underlying
buffers remain unchanged.

Cache
-----

Notice that the common case of appending text to a given piece is fast
since, the new data is simply appended to the buffer and the piece length
is increased accordingly. In order to keep the number of pieces down,
the least recently edited piece is cached and changes to it are done
in place (this is the only time buffers are modified in a non-append
only way). As a consequence they can not be undone.

Undo/redo
---------

Since the buffers are append only and the spans/pieces are never destroyed
undo/redo functionality is implemented by swapping the required spans/pieces
back in.

As illustrated above, each change to the text is recorded by an old and
a new span. An action consists of multiple changes which logically belong
to each other and should thus also be reverted together. For example
a search and replace operation is one action with possibly many changes
all over the text.

The text states can be marked by means of a snapshotting operation.
Snapshotting saves a new node to the history graph and creates a fresh
Action to which future changes will be appended until the next snapshot.

Actions make up the nodes of a connected digraph, each representing a state
of the file at some time during the current editing session. The edges of the
digraph represent state transitions that are supported by the editor. The edges
are implemented as four Action pointers (`prev`, `next`, `earlier`, and `later`).

The editor operations that execute the four aforementioned transitions
are `undo`, `redo`,`earlier`, and `later`, respectively. Undo and
redo behave in the traditional manner, changing the state one Action
at a time. Earlier and later, however, traverse the states in chronological
order, which may occasionally involve undoing and redoing many Actions at once.

Marks
-----

Because we are working with a persistent data structure marks can be
represented as pointers into the underlying (append only) buffers.
To get the position of an existing mark it suffices to traverse the
list of pieces and perform a range query on the associated buffer
segments. This also nicely integrates with the undo/redo mechanism.
If a span is swapped out all contained marks (pointers) become invalid
because they are no longer reachable from the piece chain. Once an
action is undone, and the corresponding span swapped back in, the
marks become visible again. No explicit mark management is necessary.

Properties
----------

The main advantage of the piece chain as described above is that all
operations are performed independent of the file size but instead linear
in the number of pieces i.e. editing operations. The original file buffer
never changes which means the `mmap(2)` can be performed read only which
makes optimal use of the operating system's virtual memory / paging system.

The maximum editable file size is limited by the amount of memory a process
is allowed to map into its virtual address space, this shouldn't be a problem
in practice. The whole process assumes that the file can be used as is.
In particular the editor assumes all input and the file itself is encoded
as UTF-8. Supporting other encodings would require conversion using `iconv(3)`
or similar upon loading and saving the document.

Similarly the editor has to cope with the fact that lines can be terminated
either by `\n` or `\r\n`. There is no conversion to a line based structure in
place. Instead the whole text is exposed as a sequence of bytes. All
addressing happens by means of zero based byte offsets from the start of
the file.

The main disadvantage of the piece chain data structure is that the text
is not stored contiguous in memory which makes seeking around somewhat
harder. This also implies that standard library calls like the `regex(3)`
functions can not be used as is. However this is the case for all but
the most simple data structures used in text editors.

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
