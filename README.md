Vis a vim-like text editor
==========================

[![Build
status](https://travis-ci.org/martanne/vis.svg?branch=master)](https://travis-ci.org/martanne/vis)

Vis aims to be a modern, legacy free, simple yet efficient vim-like editor.

It extends vim's modal editing with built-in support for multiple
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

Efficient syntax highlighting is provided using Parsing Expression Grammars
which can be conveniently expressed using Lua in form of LPeg.

The editor core is written in a reasonable amount of clean (your mileage
may vary), modern and legacy free C code enabling it to run in resource
constrained environments. The implementation should be easy to hack on
and encourage experimentation (e.g. native built in support for multiple
cursors). There also exists a Lua API for in process extensions.

Vis strives to be *simple* and focuses on its core task: efficient text
management. As an example the file open dialog is provided by an independent
utility. There exist plans to use a client/server architecture, delegating
window management to your windowing system or favorite terminal multiplexer.

The intention is *not* to be bug for bug compatible with vim, instead a
similar editing experience should be provided. The goal could thus be
summarized as "80% of vim's features implemented in roughly 1% of the code".

[![vis demo](https://asciinema.org/a/41361.png)](https://asciinema.org/a/41361)

Getting started / Build instructions
====================================

In order to build vis you will need a C99 compiler as well as:

 * a C library, we recommend [musl](http://www.musl-libc.org/)
 * [libcurses](http://www.gnu.org/software/ncurses/), preferably in the
   wide-character version
 * [libtermkey](http://www.leonerd.org.uk/code/libtermkey/)
 * [lua](http://www.lua.org/) >= 5.2 (optional)
 * [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) >= 0.12
   (optional runtime dependency required for syntax highlighting)

Assuming these dependencies are met, execute:

    $ ./configure && make && sudo make install

By default the `configure` script will try to auto detect support for
Lua. See `configure --help` for a list of supported options. You can
also manually tweak the generated `config.mk` file.

On Linux based systems `make standalone` will attempt to download,
compile and install all of the above dependencies into a subfolder
inorder to build a self contained statically linked binary.

`make local` will do the same but only for libtermkey, Lua and LPeg
(i.e. the system C and curses libraries are used).

Or simply use one of the distribution provided packages:

 * [ArchLinux](http://www.archlinux.org/packages/?q=vis)
 * [Alpine Linux](https://pkgs.alpinelinux.org/packages?name=vis)
 * [Void Linux](https://github.com/voidlinux/void-packages/tree/master/srcpkgs/vis)
 * [pkgsrc](http://pkgsrc.se/wip/vis-editor)

Editing Features
================

The following section gives a quick overview over the currently
supported features.

###  Operators

    c   (change)
    d   (delete)
    !   (filter)
    =   (format using fmt(1))
    gu  (make lowercase)
    gU  (make uppercase)
    J   (join)
    p   (put)
    <   (shift-left)
    >   (shift-right)
    ~   (swap case)
    y   (yank)

Operators can be forced to work line wise by specifying `V`.

### Movements

    0        (start of line)
    b        (previous start of a word)
    B        (previous start of a WORD)
    $        (end of line)
    e        (next end of a word)
    E        (next end of a WORD)
    F{char}  (to next occurrence of char to the left)
    f{char}  (to next occurrence of char to the right)
    ^        (first non-blank of line)
    g0       (begin of display line)
    g$       (end of display line)
    ge       (previous end of a word)
    gE       (previous end of a WORD)
    gg       (begin of file)
    G        (goto line or end of file)
    gj       (display line down)
    gk       (display line up)
    g_       (last non-blank of line)
    gm       (middle of display line)
    |        (goto column)
    h        (char left)
    H        (goto top/home line of window)
    j        (line down)
    k        (line up)
    l        (char right)
    L        (goto bottom/last line of window)
    `{mark}  (go to mark)
    '{mark}  (go to start of line containing mark)
    %        (match bracket)
    M        (goto middle line of window)
    ]]       (next end of C-like function)
    }        (next paragraph)
    )        (next sentence)
    ][       (next start of C-like function)
    N        (repeat last search backwards)
    n        (repeat last search forward)
    []       (previous end of C-like function)
    {        (previous paragraph)
    (        (previous sentence)
    [[       (previous start of C-like function)
    ;        (repeat last to/till movement)
    ,        (repeat last to/till movement but in opposite direction)
    #        (search word under cursor backwards)
    *        (search word under cursor forwards)
    T{char}  (till before next occurrence of char to the left)
    t{char}  (till before next occurrence of char to the right)
    ?{text}  (to next match of text in backward direction)
    /{text}  (to next match of text in forward direction)
    w        (next start of a word)
    W        (next start of a WORD)

  An empty line is currently neither a word nor a WORD.

  Some of these commands do not work as in vim when prefixed with a
  digit i.e. a multiplier. As an example in vim `3$` moves to the end
  of the 3rd line down. However vis treats it as a move to the end of
  current line which is repeated 3 times where the last two have no
  effect.

### Text objects

  All of the following text objects are implemented in an inner variant
  (prefixed with `i`) and a normal variant (prefixed with `a`):

    w  word
    W  WORD
    s  sentence
    p  paragraph
    [,], (,), {,}, <,>, ", ', `         block enclosed by these symbols

  For sentence and paragraph there is no difference between the
  inner and normal variants.

    gn      matches the last used search term in forward direction
    gN      matches the last used search term in backward direction

  Additionally the following text objects, which are not part of stock vim
  are also supported:

    ae      entire file content
    ie      entire file content except for leading and trailing empty lines
    af      C-like function definition including immediately preceding comments
    if      C-like function definition only function body
    al      current line
    il      current line without leading and trailing white spaces

### Modes

  Vis implements more or less functional normal, operator-pending, insert,
  replace and visual (in both line and character wise variants) modes.

  Visual block mode is not implemented and there exists no immediate
  plan to do so. Instead vis has built in support for multiple cursors.

  Command mode is implemented as a regular file. Use the full power of the
  editor to edit your commands / search terms.

  Ex mode is deliberately not implemented, instead a variant of the structural
  regular expression based command language of `sam(1)` is supported.

### Multiple Cursors / Selections

  vis supports multiple cursors with immediate visual feedback (unlike
  in the visual block mode of vim where for example inserts only become
  visible upon exit). There always exists one primary cursor located
  within the current view port. Additional cursors ones can be created
  as needed. If more than one cursor exists, the primary one is blinking.

  To manipulate multiple cursors use in normal mode:
  
    Ctrl-K       create count new cursors on the lines above
    Ctrl-Meta-K  create count new cursors on the lines above the first cursor
    Ctrl-J       create count new cursors on the lines below
    Ctrl-Meta-J  create count new cursors on the lines below the last cursor
    Ctrl-P       remove primary cursor
    Ctrl-N       select word the cursor is currently over, switch to visual mode
    Ctrl-U       make the count previous cursor primary
    Ctrl-D       make the count next cursor primary
    Ctrl-C       remove the count cursor column
    Ctrl-L       remove all but the count cursor column
    Tab          try to align all cursor on the same column
    Esc          dispose all but the primary cursor

  Visual mode was enhanced to recognize:
    
    I            create a cursor at the start of every selected line
    A            create a cursor at the end of every selected line
    Tab          left align selections by inserting spaces
    Shift-Tab    right align selections by inserting spaces
    Ctrl-N       create new cursor and select next word matching current selection
    Ctrl-X       clear (skip) current selection, but select next matching word
    Ctrl-P       remove primary cursor
    Ctrl-U/K     make the count previous cursor primary
    Ctrl-D/J     make the count next cursor primary
    Ctrl-C       remove the count cursor column
    Ctrl-L       remove all but the count cursor column
    +            rotates selections rightwards count times
    -            rotates selections leftwards count times
    \            trim selections, remove leading and trailing white space
    Esc          clear all selections, switch to normal mode

  In insert/replace mode

    Shift-Tab    align all cursors by inserting spaces

### Marks

    [a-z] general purpose marks
    <     start of the last selected visual area in current buffer
    >     end of the last selected visual area in current buffer

  No marks across files are supported. Marks are not preserved over
  editing sessions.

### Registers

  Supported registers include:

    "a-"z   general purpose registers
    "A-"Z   append to corresponding general purpose register
    "*, "+  system clipboard integration via shell script vis-clipboard
    "0      yank register
    "/      search register
    ":      command register
    "_      black hole (/dev/null) register

  If no explicit register is specified a default register is used.

### Undo/Redo and Repeat

  The text is currently snapshotted whenever an operator is completed as
  well as when insert or replace mode is left. Additionally a snapshot
  is also taken if in insert or replace mode a certain idle time elapses.

  Another idea is to snapshot based on the distance between two consecutive
  editing operations (as they are likely unrelated and thus should be
  individually reversible).

  Besides the regular undo functionality, the key bindings `g+` and `g-`
  traverse the history in chronological order. Further more the `:earlier`
  and `:later` commands provide means to restore the text to an arbitrary
  state.

  The repeat command `.` works for all operators and is able to repeat
  the last insertion or replacement.

### Macros

  The general purpose registers `[a-z]` can be used to record macros. Use
  one of `[A-Z]` to append to an existing macro. `q` starts a recording,
  `@` plays it back. `@@` refers to the least recently recorded macro.
  `@:` repeats the last :-command. `@/` is equivalent to `n` in normal mode.

### Structural Regular Expression based Command Language

  Vis supports [sam's](http://sam.cat-v.org/)
  [structural regular expression](http://doc.cat-v.org/bell_labs/structural_regexps/)
  based [command language](http://doc.cat-v.org/bell_labs/sam_lang_tutorial/).

  The basic command syntax supported is mostly compatible with the description
  found in the [sam manual page](http://man.cat-v.org/plan_9/1/sam).
  The [sam reference card](http://sam.cat-v.org/cheatsheet/) might also be useful.

  Sam commands can be entered from the vis prompt as `:<cmd>`

  A command behaves differently depending on the mode in which it is issued:

   - in visual mode it behaves as if an implicit extract x command
     matching the current selection(s) would be preceding it. That is
     the command is executed once for each selection.

   - in normal mode:

      * if an address for the command was provided it is evaluated starting
        from the current cursor position(s) i.e. dot is set to the current
        cursor position.

      * if no address was supplied to the command then:

         + if multiple cursors exist, the command is executed once for every
           cursor with dot set to the current line of the cursor

         + otherwise if there is only 1 cursor then the command is executed
           with dot set to the whole file

  The command syntax was slightly tweaked to accept more terse commands.

  - When specifying text or regular expressions the trailing delimiter can
    be elided if the meaning is unambiguous.

  - If only an address is provided the print command will be executed.

  - The print command creates a selection matching its range.

  - In text entry `\t` inserts a literal tab character (sam only recognizes `\n`).

  Hence the sam command `,x/pattern/` can be abbreviated to `x/pattern`

  If after a command no selections remain, the editor will switch to normal
  mode otherwise it remains in visual mode.

  Other differences compared to sam include:

  - The following commands are deliberately not implemented:

     * move `m`
     * copy `t`
     * print line address `=`
     * print character address `=#`
     * set current file mark `k`
     * undo `u`

  - Multi file support is currently very primitive:

     * the "regexp" construct to evaluate an address in a file matching
       regexp is currently not supported.

     * the following commands related to multiple file editing are not
       supported: `b`, `B`, `n`, `D`, `f`.

  - The special grouping semantics where all commands of a group operate
    on the the same state is not implemented.

  - The file mark address `'` (and corresponding `k` command) is not supported

### Command line prompt

  Besides the sam command language the following commands are also recognized
  at the `:`-command prompt. Any unique prefix can be used.

    :bdelete      close all windows which display the same file as the current one
    :earlier      revert to older text state
    :e            replace current file with a new one or reload it from disk
    :langmap      set key equivalents for layout specific key mappings
    :later        revert to newer text state
    :!            launch external command, redirect keyboard input to it
    :map          add a global key mapping
    :map-window   add a window local key mapping
    :new          open an empty window, arrange horizontally
    :open         open a new window
    :qall         close all windows, exit editor
    :q            close currently focused window
    :r            insert content of another file at current cursor position
    :set          set the options below
    :split        split window horizontally
    :s            search and replace currently implemented in terms of `sed(1)`
    :unmap        remove a global key mapping
    :unmap-window remove a window local key mapping
    :vnew         open an empty window, arrange vertically
    :vsplit       split window vertically
    :wq           write changes then close window
    :w            write current buffer content to file

     tabwidth   [1-8]           default 8

       set display width of a tab and number of spaces to use if
       expandtab is enabled

     expandtab  (yes|no)        default no

       whether typed in tabs should be expanded to tabwidth spaces

     autoindent (yes|no)        default no

       replicate spaces and tabs at the beginning of the line when
       starting a new line.

     number         (yes|no)    default no
     relativenumber (yes|no)    default no

       whether absolute or relative line numbers are printed alongside
       the file content

     syntax      name           default yes

       use syntax definition given (e.g. "c") or disable syntax
       highlighting if no such definition exists (e.g :set syntax off)

     show

       show/hide special white space replacement symbols

       newlines = [0|1]         default 0
       tabs     = [0|1]         default 0
       spaces   = [0|1]         default 0

     cursorline (yes|no)        default no

       highlight the line on which the cursor currently resides

     colorcolumn number         default 0

       highlight the given column

     theme      name            default dark-16.lua | solarized.lua (16 | 256 color)

       use the given theme / color scheme for syntax highlighting

  Commands taking a file name will use a simple file open dialog based
  on the `vis-open` shell script, if given a file pattern or directory.

    :e *.c          # opens a menu with all C files
    :e .            # opens a menu with all files of the current directory

### Configuring vis: visrc.lua, and environment variables

Settings and keymaps can be specified in a `visrc.lua` file, which will
be read by `vis` at runtime. An example `visrc.lua` file is installed
in `/usr/local/share/vis` by default. This file can be copied to
`$XDG_CONFIG_HOME/vis` (which defaults to `$HOME/.config/vis`) for
further configuration.

The environment variable `VIS_PATH` can be set to override the path
that `vis` will look for Lua support files as used for syntax
highlighting. `VIS_PATH` defaults (in this order) to

- The location of the `vis` binary
- `$XDG_CONFIG_HOME/vis`, where `$XDG_CONFIG_HOME` refers to
`$HOME/.config` if unset
- `/usr/local/share/vis`
- `/usr/share/vis`

The environment variable `VIS_THEME` can be set to specify the
theme used by `vis` e.g.

    VIS_THEME=/path/to/your/theme.lua
    export VIS_THEME

### Runtime Configurable Key Bindings

Vis supports run time key bindings via the `:{un,}map{,-window}` set of
commands. The basic syntax is:

    :map <mode> <lhs> <rhs>

where mode is one of `normal`, `insert`, `replace`, `visual`,
`visual-line` or `operator-pending`. lhs refers to the key to map, rhs is
a key action or alias. An existing mapping can be overridden by appending
`!` to the map command.

Key mappings are always recursive, this means doing something like:

    :map! normal j 2j

will not work because it will enter an endless loop. Instead vis uses
pseudo keys referred to as key actions which can be used to invoke a set
of available (see :help or <F1> for a list) editor functions. Hence the
correct thing to do would be:

    :map! normal j 2<cursor-line-down>

Unmapping works as follows:

    :unmap <mode> <lhs>

The commands suffixed with `-window` only affect the currently active window.

### Layout Specific Key Bindings

Vis allows to set key equivalents for non-latin keyboard layouts. This
facilitates editing non-latin texts. The defined mappings take effect
in all non-input modes, i.e. everywhere except in insert and replace mode.

For example, the following maps the movement keys in Russian layout:

    :langmap ролд hjkl

More generally the syntax of the `:langmap` command is:

    :langmap <sequence of keys in your layout> <sequence of equivalent keys in latin layout>

If the key sequences have not the same length, the rest of the longer
sequence will be discarded.

### Tab <-> Space conversion and Line endings \n vs \r\n

  Tabs can optionally be expanded to a configurable number of spaces.
  The first line ending in the file determines what will be inserted
  upon a line break (defaults to \n).

### Jump list and change list

  A per window, file local jump list (navigate with `CTRL+O` and `CTRL+I`)
  and change list (navigate with `g;` and `g,`) is supported. The jump
  list is implemented as a fixed sized ring buffer.

### Mouse support

  The mouse is currently not used at all.

### Non Goals

  Some of the features of vim which will *not* be implemented:

   - tabs / multiple workspaces / advanced window management
   - file and directory browser
   - support for file archives (tar, zip, ...)
   - support for network protocols (ftp, http, ssh ...)
   - encryption
   - compression
   - GUIs (neither x11, motif, gtk, win32 ...) although the codebase
     should make it easy to add them
   - VimL
   - plugins (certainly not vimscript, if anything it should be lua based)
   - right-to-left text
   - ex mode
   - diff mode
   - vimgrep
   - internal spell checker
   - compile time configurable features / `#ifdef` mess

Lua API for in process extension
================================

Vis provides a simple Lua API for in process extension. At startup the
`visrc.lua` file is executed, this can be used to register a few event
callbacks which will be invoked from the editor core. While executing
these user scripts the editor core is blocked, hence it is intended for
simple short lived (configuration) tasks.

At this time there exists no API stability guarantees.

 - `vis`
   - `MODE_NORMAL`, `MODE_OPERATOR_PENDING`, `MODE_INSERT`, `MODE_REPLACE`, `MODE_VISUAL`, `MODE_VISUAL_LINE` mode constants
   - `lexers` LPeg lexer support module
   - `events` hooks
     - `start()`
     - `quit()`
     - `win_open(win)`
     - `win_close(win)`
   - `files()` iterator
   - `windows()` iterator
   - `command(cmd)`
   - `info(msg)`
   - `open(filename)`
   - `textobject_register(function)` register a Lua function as a text object, returns associated `id` or `-1`
   - `textobject(id)` select/execute a text object
   - `motion_register(function)` register a Lua function as a motion, returns associated `id` or `-1`
   - `motion(id)` select/execute a motion
   - `map(mode, key, function)` map a Lua function to `key` in `mode`
 - `file`
   - `content(pos, len)`
   - `insert(pos, data)`
   - `delete(pos, len)`
   - `lines_iterator()`
   - `name`
   - `lines[0..#lines+1]` array giving read/write access to lines
 - `window`
   - `file`
   - `syntax` lexer name used for syntax highlighting or `nil`
   - `cursor`
     - `line` (1 based), `col` (0 based)
     - `to(line, col)`
     - `pos` bytes from start of file (0 based)

Most of the exposed objects are managed by the C core. Allthough there
is a simple object life time management mechanism in place, it is still
recommended to *not* let the Lua objects escape from the event handlers
(e.g. by assigning to global Lua variables).

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

Syntax Highlighting using Parsing Expression Grammars
=====================================================

[Parsing Expression Grammars](https://en.wikipedia.org/wiki/Parsing_expression_grammar)
(PEG) have the nice property that they are closed under composition.
In the context of an editor this is useful because lexers can be
embedded into each other, thus simplifying syntax highlighting
definitions.

Vis reuses the [Lua](http://www.lua.org/) [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/)
based lexers from the [Scintillua](http://foicica.com/scintillua/) project.

Future Plans / Ideas
====================

This section contains some ideas for further architectural changes.

Event loop with asynchronous I/O
--------------------------------

The editor core should feature a proper main loop mechanism supporting
asynchronous non-blocking and always cancelable tasks which could be
used for all possibly long lived actions such as:

 - `!`, `=` operators
 - `:substitute` and `:write` commands
 - code completion
 - compiler integration (similar to vim's quick fix functionality)

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
 `visrc.lua`         | Lua startup and configuration script

Testing infrastructure for the [low level core data structures]
(https://github.com/martanne/vis/tree/test/test/core), [vim compatibility]
(https://github.com/martanne/vis/tree/test/test/vim) and [vis specific features]
(https://github.com/martanne/vis/tree/test/test/vis) is in place, but
lacks proper test cases.
