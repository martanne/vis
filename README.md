Vis a vim-like text editor
==========================

Vis aims to be a modern, legacy free, simple yet efficient vim-like editor.

As an universal editor it has decent Unicode support (including double width
and combining characters) and should cope with arbitrary files including:

 - large (up to a few Gigabytes) ones including
   - Wikipedia/OpenStreetMap XML / SQL / CVS dumps
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

![vis demo](https://raw.githubusercontent.com/martanne/vis/gh-pages/screencast.gif)

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

Editing Features
================

The following section gives a quick overview over the currently
supported features.

###  Operators

    d   (delete)
    c   (change)
    y   (yank)
    p   (put)
    >   (shift-right)
    <   (shift-left)
    J   (join)
    ~   (swap case)
    gu  (make lowercase)
    gU  (make uppercase)
    !   (filter)
    =   (format using fmt(1))

Operators can be forced to work line wise by specifying `V`.

### Movements

    h        (char left)
    l        (char right)
    j        (line down)
    k        (line up)
    gj       (display line down)
    gk       (display line up)
    0        (start of line)
    ^        (first non-blank of line)
    g_       (last non-blank of line)
    $        (end of line)
    %        (match bracket)
    b        (previous start of a word)
    B        (previous start of a WORD)
    w        (next start of a word)
    W        (next start of a WORD)
    e        (next end of a word)
    E        (next end of a WORD)
    ge       (previous end of a word)
    gE       (previous end of a WORD)
    {        (previous paragraph)
    }        (next paragraph)
    (        (previous sentence)
    )        (next sentence)
    [[       (previous start of C-like function)
    []       (previous end of C-like function)
    ][       (next start of C-like function)
    ]]       (next end of C-like function)
    gg       (begin of file)
    g0       (begin of display line)
    gm       (middle of display line)
    g$       (end of display line)
    G        (goto line or end of file)
    |        (goto column)
    n        (repeat last search forward)
    N        (repeat last search backwards)
    H        (goto top/home line of window)
    M        (goto middle line of window)
    L        (goto bottom/last line of window)
    *        (search word under cursor forwards)
    #        (search word under cursor backwards)
    f{char}  (to next occurrence of char to the right)
    t{char}  (till before next occurrence of char to the right)
    F{char}  (to next occurrence of char to the left)
    T{char}  (till before next occurrence of char to the left)
    ;        (repeat last to/till movement)
    ,        (repeat last to/till movement but in opposite direction)
    /{text}  (to next match of text in forward direction)
    ?{text}  (to next match of text in backward direction)
    `{mark}  (go to mark)
    '{mark}  (go to start of line containing mark)

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

  Ex mode is deliberately not implemented, use `ssam(1)` if you need a
  stream editor.
  
### Multiple Cursors / Selections

  vis supports multiple cursors with immediate visual feedback (unlike
  in the visual block mode of vim where for example inserts only become
  visible upon exit). There always exists one primary cursor located
  within the current view port. Additional cursors ones can be created
  as needed. If more than one cursor exists, the primary one is blinking.
  
  To manipulate multiple cursors use in normal mode:
  
    CTRL-K   create a new cursor on the line above
    CTRL-J   create a new cursor on the line below
    CTRL-P   remove primary cursor
    CTRL-N   select word the cursor is currently over, switch to visual mode
    CTRL-U   make the previous cursor primary
    CTRL-D   make the next cursor primary
    TAB      try to align all cursor on the same column
    ESC      if a selection is active, clear it.
             Otherwise dispose all but the primary cursor.

  Visual mode was enhanced to recognize:
    
    I        create a cursor at the start of every selected line
    A        create a cursor at the end of every selected line
    CTRL-N   create new cursor and select next word matching current selection
    CTRL-X   clear (skip) current selection, but select next matching word
    CTRL-P   remove primary cursor
    CTRL-U   make the previous cursor primary
    CTRL-D   make the next cursor primary

  In insert/replace mode

    S-Tab    aligns all cursors by inserting spaces

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

### Command line prompt

  At the `:`-command prompt only the following commands are recognized, any
  valid unique prefix can be used:

    :nnn          go to line nnn
    :bdelete      close all windows which display the same file as the current one
    :edit         replace current file with a new one or reload it from disk
    :open         open a new window
    :qall         close all windows, exit editor
    :quit         close currently focused window
    :read         insert content of another file at current cursor position
    :split        split window horizontally
    :vsplit       split window vertically
    :new          open an empty window, arrange horizontally
    :vnew         open an empty window, arrange vertically
    :wq           write changes then close window
    :xit          like :wq but write only when changes have been made
    :write        write current buffer content to file
    :saveas       save file under another name
    :substitute   search and replace currently implemented in terms of `sed(1)`
    :earlier      revert to older text state
    :later        revert to newer text state
    :map          add a global key mapping
    :unmap        remove a global key mapping
    :map-window   add a window local key mapping
    :unmap-window remove a window local key mapping
    :langmap      set key equivalents for layout specific key mappings
    :!            filter range through external command
    :|            pipe range to external command and display output in a new window
    :set          set the options below

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

  Each command can be prefixed with a range made up of a start and
  an end position as in start,end. Valid position specifiers are:

    .          start of the current line
    +n and -n  start of the line relative to the current line
    'm         position of mark m
    /pattern/  first match after current position

  If only a start position without a command is given then the cursor
  is moved to that position. Additionally the following ranges are
  predefined:

    %          the whole file, equivalent to 1,$
    *          the current selection, equivalent to '<,'>

  History support, tab completion and wildcard expansion are other
  worthwhile features. However implementing them inside the editor feels
  wrong. For now you can use the `:edit` command with a pattern or a
  directory like this.

    :e *.c
    :e .

  vis will call the `vis-open` script which invokes dmenu or slmenu
  with the files corresponding to the pattern. The file you select in
  dmenu/slmenu will be opened in vis.

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

    :unmap <lhs>

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
   - ex mode (if you need a stream editor use `ssam(1)`
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
 `text.[ch]`         | low level text / marks / {un,re}do / piece table implementation
 `text-motions.[ch]` | movement functions take a file position and return a new one
 `text-objects.[ch]` | functions take a file position and return a file range
 `text-regex.[ch]`   | text search functionality, designated place for regex engine
 `text-util.[ch]`    | text related utility functions mostly dealing with file ranges
 `view.[ch]`         | ui-independent viewport, shows part of a file, syntax highlighting, cursor placement, selection handling
 `ui.h`              | abstract interface which has to be implemented by ui backends
 `ui-curses.[ch]`    | a terminal / curses based user interface implementation
 `buffer.[ch]`       | dynamically growing buffer used for registers and macros
 `ring-buffer.[ch]`  | fixed size ring buffer used for the jump list
 `map.[ch]`          | crit-bit tree based map supporting unique prefix lookups and ordered iteration. used to implement `:`-commands
 `vis.h`             | vi(m) specific editor frontend library public API
 `vis.c`             | vi(m) specific editor frontend implementation
 `vis-core.h`        | internal header file, various structs for core editor primitives
 `vis-cmds.c`        | vi(m) `:`-command implementation
 `vis-modes.c`       | vi(m) mode switching, enter/leave event handling
 `vis-motions.c`     | vi(m) cursor motion implementation
 `vis-operators.c`   | vi(m) operator implementation
 `vis-lua.c`         | Lua bindings, exposing core vis APIs for in process extension
 `main.c`            | key action definitions, program entry point
 `config.def.h`      | definition of default key bindings (mapping of key actions)
 `visrc.lua`         | Lua startup and configuration script
 `lexers/`           | Lua LPeg based lexers used for syntax highlighting

Testing infrastructure for the [low level text manipulation routines]
(https://github.com/martanne/vis/tree/test/test/text), [vim compatibility]
(https://github.com/martanne/vis/tree/test/test/vim) and [vis specific features]
(https://github.com/martanne/vis/tree/test/test/vis) is in place, but
lacks proper test cases.
