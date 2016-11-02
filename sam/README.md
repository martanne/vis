Tests for vis - structural regular expression support
-----------------------------------------------------

We test the structural regular expression implementation by supplying
the same command to both vis and [sam](http://sam.cat-v.org/). More
concretely we use `ssam(1)` the non-graphical streaming interface to
sam. These tests are intended to be run on a system with
[9base](http://tools.suckless.org/9base) or
[plan9port](https://swtch.com/plan9port/) installed.

However, be aware that vis does deliberately not implement all commands
available in sam. Moreover, some language constructs have slightly
different semantics. Of particular relevance is the different grouping
implementation. In sam each command of a group is executed with
the same initial file state and all changes are later applied in parallel
(if no conflict occurred). Whereas vis applies the changes immediately
and subsequent commands will operate on the modified file content.
As a consequence the following snippet to swap two words does currently
not work in vis:

    ,x/[a-zA-Z]+/{
      g/Emacs/ v/....../ c/vi/
      g/vi/ v/.../ c/Emacs/
    }

This is particularly relevant for the tests because they are all
executed in an implicit group.

A test constitutes of 3 files, the first 2 of which are mandatory:

 * `test.in` the file/buffer content with which the editor is started
 * `test.cmd` a file containing the structural regular expression
    command as you would type it in vis at the `:`-prompt or pass to
    `ssam(1)`.
 * `test.ref` if omitted, the output from sam will be considered as
    reference. 

The top level shell script `test.sh` looks for these files in sub
directories, executes both editors and compares the resulting output.

Type `make` to run all tests.
