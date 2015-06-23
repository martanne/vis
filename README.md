Why another text editor?
========================

It all started when I was recently reading the excellent
[Project Oberon](http://www.inf.ethz.ch/personal/wirth/ProjectOberon/),
where in chapter 5 a data structure for managing text is introduced.
I found this rather appealing and wanted to see how it works in practice.

After some time I decided that besides just having fun hacking around I
might as well build something which could (at least in the long run)
replace my current editor of choice: vim.

This should be accomplished by a reasonable amount of clean (your mileage
may vary), modern and legacy free C code. Certainly not an old,
[500'000 lines long](https://www.openhub.net/p/vim) #ifdef cluttered
mess which tries to run on all broken systems ever envisioned by mankind.

Admittedly vim has a lot of functionally, most of which I don't use. I
therefore set out with the following main goals:

 - Unicode aware

 - binary clean

 - handle arbitrary files (this includes large ones, think >100M SQL-dumps)

 - unlimited undo/redo support

 - syntax highlighting

 - regex search (and replace)

 - multiple file/window support

The goal could thus be summarized as "80% of vim's features (in other
words the useful ones) implemented in roughly 1% of the code".

Finally and most importantly it is fun! Writing a text editor presents
some interesting challenges and design decisions, some of which are
explained below.

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
all over the text. Actions are saved in a tree structure.

A state of the text can be marked by means of a snapshotting operation.
Snapshotting saves a new node to the history tree and creates a fresh Action
to which future changes will be appended until the next snapshot.
The undo/redo functionality operates on such marked states and switches
between them by traversing adjacent nodes in the tree.

Undo and Redo move the state of the document up and down on a single branch
of the history tree, but other branches can be accessed with the Earlier
and Later commands, which iterate through snapshotted states chronologically.

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
or similar upon loading and saving the document, which defeats the whole
purpose.

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

Screen Drawing
==============

The current code takes a rather simple approach to screen drawing. It
basically only remembers the starting position of the area being shown.
Then fetches a "screen full" of bytes and outputs one character at a
time until the end of the window is reached. A consequence of this
approach is that lines are always wrapped and horizontal scrolling is
not supported.

No efforts are made to reduce the terminal output. This task is delegated
to the underlying curses library which already performs a kind of double
buffering. The window is always redrawn completely even if only a single
character changes. It turns out this is actually necessary if one wants
to support multiline syntax highlighting.

While outputting the individual characters a cell matrix is populated
where each entry stores the length in bytes of the character displayed
at this particular cell. For characters spanning multiple columns the
length is always stored in the leftmost cell. As an example a tab has a
length of 1 byte followed by up to 7 cells with a length of zero.
Similarly a `\r\n` line ending occupies only one screen cell but has a
length of 2.

This matrix is actually stored per line inside a double linked list of
structures representing screen lines. For each line we keep track of
the length in bytes of the underlying text, the display width of all
characters part of the line, and the logical line number.

All cursor positioning is always performed in bytes from the start of
the file and works by traversing the double linked list of screen lines
until the correct line is found. Then the cell array is consulted to
move to the correct column.

Syntax-Highlighting
-------------------

The editor takes a similar regex-based approach to syntax highlighting
than sandy and reuses its syntax definitions but always applies them to
a "screen full" of text thus enabling multiline coloring.

Window-Management
-----------------

It is possible to open multiple windows via the `:split/:vsplit/:open`
commands or by passing multiple files on the command line.

In principle it would be nice to follow a similar client/server approach
as sam/samterm i.e. having the main editor as a server and each window
as a separate client process with communication over a unix domain socket.

That way window management would be taken care of by dwm or dvtm and the
different client processes would still share common cut/paste registers
etc.

However at the moment I don't want to open that can of worms and instead
settled for a single process architecture.

Search and replace
------------------

Currently the editor copies the whole text to a contiguous memory block
and then uses the standard regex functions from libc. Clearly this is not
a satisfactory solution for large files and kind of defeats the whole
effort spent on the piece table.

The long term solution is to write our own regular expression engine or
modify an existing one to make use of the iterator API. This would allow
efficient search without having to double memory consumption.

The used regex engine should use a non-backtracking algorithm. Useful
resources include:

 - [Russ Cox's regex pag](http://swtch.com/~rsc/regexp/)
 - [TRE](https://github.com/laurikari/tre) as
   [used by musl](http://git.musl-libc.org/cgit/musl/tree/src/regex)
   which uses a parallel [TNFA matcher](http://laurikari.net/ville/spire2000-tnfa.ps)
 - [Plan9's regex library](http://plan9.bell-labs.com/sources/plan9/sys/src/libregexp/)
   which has its root in Rob Pike's sam text editor
 - [RE2](https://github.com/google/re2) C++ regex library

Command-Prompt
--------------

The editor needs some form of command prompt to get user input
(think `:`, `/`, `?` in vim).

At first I wanted to implement this in terms of an external process,
similar to the way it is done in sandy with communication back to the
editor via a named pipe.

At some point I thought it would be possible to provide all editor commands
as shell scripts in a given directory, then set $PATH accordingly and run
the shell. This would give us readline editing, tab completion, history and
Unicode support for free. But unfortunately it won't work due to quoting
issues and other conflicts of special symbols with different meanings.

Later it occurred to me that the editor prompt could just be treated as
special 1 line file. That is, all the main editor functionality is reused
with a slightly different set of key bindings.

This approach also has the added benefit of further testing the main editor
component (in particular corner cases like editing at the end of the file).

Editor Frontends
================

The editor core is written in a library like fashion which should make
it possible to write multiple frontends with possibly different user
interfaces/paradigms. The frontend to run is selected based on the
executable name.

The default, and currently only, interface is a vim clone called vis.

Key binding modes
-----------------

The available key bindings for the different modes are arranged in a
hierarchical way in config.h (there is also an ascii tree giving an
overview in that file). Each mode can specify a parent mode which is
used to look up a key binding if it is not found in the current mode.
This reduces redundancy for keys which have the same meaning in
different modes.

Each mode can also provide hooks which are executed upon entering/leaving
the mode and when there was an unmatched key.

vis a vim-like frontend
-----------------------

The following section gives a quick overview over various vim features
and their current support in vis.

###  Operators

    d   (delete)
    c   (change)
    y   (yank)
    p   (put)
    >   (shift-right)
    <   (shift-left),
    J   (join)
    ~   (swap case)
    gu  (make lowercase)
    gU  (make uppercase)

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
    gg       (begin of file)
    G        (goto line or end of file)
    |        (goto column)
    n        (repeat last search forward)
    N        (repeat last search backwards)
    f{char}  (to next occurrence of char to the right)
    t{char}  (till before next occurrence of char to the right)
    F{char}  (to next occurrence of char to the left)
    T{char}  (till before next occurrence of char to the left)
    ;        (repeat last to/till movement)
    ,        (repeat last to/till movement but in opposite direction)
    /{text}  (to next match of text in forward direction)
    ?{text}  (to next match of text in backward direction)

  An empty line is currently neither a word nor a WORD.

  The semantics of a paragraph and a sentence is also not always 100%
  the same as in vim.

  Some of these commands do not work as in vim when prefixed with a
  digit i.e. a multiplier. As an example in vim `3$` moves to the end
  of the 3rd line down. However vis treats it as a move to the end of
  current line which is repeated 3 times where the last two have no
  effect.

  In general there are still a lot of improvements to be made in the
  case movements are forced to be line or character wise. Also some of
  them should be inclusive in some context and exclusive in others.
  At the moment they always behave the same.

### Text objects

  All of the following text objects are implemented in an inner variant
  (prefixed with 'i') and a normal variant (prefixed with 'a'):

    w  word
    W  WORD
    s  sentence
    p  paragraph
    [,], (,), {,}, <,>, ", ', `         block enclosed by these symbols

  For sentence and paragraph there is no difference between the
  inner and normal variants.

### Modes

  At the moment there exists a more or less functional insert, replace
  and visual mode (in both line and character wise variants).

### Marks

    [a-z] general purpose marks
    <     start of the last selected visual area in current buffer
    >     end of the last selected visual area in current buffer

  No marks across files are supported. Marks are not preserved over
  editing sessions.

### Registers

  Only the 26 lower case registers `[a-z]` and 1 additional default register
  is supported.

### Undo/Redo and Repeat

  The text is currently snapshotted whenever an operator is completed as
  well as when insert or replace mode is left. Additionally a snapshot
  is also taken if in insert or replace mode a certain idle time elapses.

  Another idea is to snapshot based on the distance between two consecutive
  editing operations (as they are likely unrelated and thus should be
  individually reversible).

  The repeat command `.` works for all operators and is able to repeat
  the last insertion or replacement.

### Macros

`[a-z]` are recoginized macro names, `q` starts a recording, `@` plays it back.
`@@` refers to the least recently recorded macro.

### Command line prompt

  At the `:`-command prompt only the following commands are recognized:

    :nnn     go to line nnn
    :bdelete close all windows which display the same file as the current one
    :edit    replace current file with a new one or reload it from disk
    :open    open a new window
    :qall    close all windows, exit editor
    :quit    close currently focused window
    :read    insert content of another file at current cursor position
    :split   split window horizontally
    :vsplit  split window vertically
    :new     open an empty window, arrange horizontally
    :vnew    open an empty window, arrange vertically
    :wq      write changes then close window
    :xit     like :wq but write only when changes have been made
    :write   write current buffer content to file
    :saveas  save file under another name
    :set     set the options below

     tabwidth   [1-8]

       set display width of a tab and number of spaces to use if
       expandtab is enabled

     expandtab  (yes|no)

       whether typed in tabs should be expanded to tabwidth spaces

     autoindent (yes|no)

       replicate spaces and tabs at the beginning of the line when
       starting a new line.

     number         (yes|no)
     relativenumber (yes|no)

       whether absolute or relative line numbers are printed alongside
       the file content

     syntax      name

       use syntax definition given (e.g. "c") or disable syntax
       highlighting if no such definition exists (e.g :set syntax off)

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

  The substitute command is recognized but not yet implemented. The `!`
  command to filter text through an external program is also planned.

  History support, tab completion and wildcard expansion are other
  worthwhile features. However implementing them inside the editor
  feels wrong.

### Tab <-> Space and Line endings \n vs \r\n

  Tabs can optionally be expaned to a configurable number of spaces.
  The first line ending in the file determines what will be inserted
  upon a line break (defaults to \n).

### Jump list and change list

  A per window, file local jump list (navigate with `CTRL+O` and `CTRL+I`)
  and change list (navigate with `g;` and `g,`) is supported. The jump
  list is implemented as a fixed sized ring buffer.

### Mouse support

  The mouse is currently not used at all.

### Other features

 Things which I would like to add in the long term are:

   + code completion: this should be done as an external process. I will
     have to take a look at the tools from the llvm / clang project. Maybe
     dvtm's terminal emulation support could be reused to display an
     slmenu inside the editor at the cursor position?

   + something similar to vim's quick fix functionality

 Things I might add

   + runtime configurable key bindings
   + visual block mode / multiple selections
   + text folding

 Stuff which vim does which I don't use and have no plans to add:

   - GUIs (neither x11, motif, gtk, win32 ...)
   - plugins (certainly not vimscript, if anything it should be lua based)
   - right-to-left text
   - tabs (as in multiple workspaces)
   - ex mode

How to help?
------------

At this point it might be best to fetch the code, edit some scratch file,
notice an odd behavior or missing functionality, write and submit a patch
for it, then iterate.

WARNING: There are probably still some bugs left which could corrupt your
         unsaved changes. Use at your own risk. At this point I suggest to
         only edit non-critical files which are under version control and
         thus easily recoverable!

A quick overview over the code structure to get you started:

 File(s)             | Description
 ------------------- | -----------------------------------------------------
 `text.[ch]`         | low level text / marks / {un,re}do / piece table implementation
 `text-motions.[ch]` | movement functions take a file position and return a new one
 `text-objects.[ch]` | functions take a file position and return a file range
 `vis.c`             | vi(m) specific editor frontend, program entry point
 `editor.[ch]`       | editor window management
 `view.[ch]`         | ui-independent viewport, shows part of a file, syntax highlighting, cursor placement
 `ui.h`              | abstract interface as implemented by user interface
 `ui-curses.[ch]`    | a terminal / curses based user interface implementation
 `buffer.[ch]`       | dynamically growing buffer used for registers and macros
 `ring-buffer.[ch]`  | fixed size ring buffer used for the jump list
 `map.[ch]`          | crit-bit tree based map supporting unique prefix lookups and ordered iteration. used to implement `:`-commands.
 `config.def.h`      | definition of key bindings, commands, syntax highlighting

Hope this gets the interested people started.

TODO
----

Here is an incomplete list of TODO items and/or ideas for further work
in no particular order:

 * Review and cleanup the existing implementation (e.g. selection handling)
    - Eliminate global state and expose vis frontend as "library"
 * Implement `:!` using a proper (libuv based?) mainloop
 * Implement `:substitute`
 * Bugfix: editing the same file in multiple windows can cause "corruption"
 * Review/Implement cindent mode #33
 * Add history support to `:`-prompt
 * Implement wordwrap (i.e `gq` and `:set textwidth`) using `fmt(1)` ?
 * Overhaul key bindings to support runtime configuration / streamline config.def.h
 * Implement/review/merge history undo tree
 * Implement a regex engine which works with the iterator API
 * Write [unit test](http://ccodearchive.net/info/tap.html) for the low
   level `text_*` interface
 * Improve syntax highlighting, investigate whether already existing
   syntax definitions from other editors could be reused
 * Optimize `text_delete` in case of consecutive delete operations
 * Add a RPC interface, experiment with a client/server architecture and
   delegate window management to dwm/dvtm

Feel free to ask questions if something is unclear! There are still a lot
of bugs left to fix, but by now I'm fairly sure that the general concept
should work.

As always, comments and patches welcome!

Cheers,
Marc
