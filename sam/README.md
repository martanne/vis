Tests for vis - structural regular expression support
-----------------------------------------------------

We test the structural regular expression implementation by supplying
the same command to both vis and [sam](http://sam.cat-v.org/). More
concretely, we use `ssam(1)` the non-graphical streaming interface to
sam. These tests are intended to be run on a system with
[9base](http://tools.suckless.org/9base) or
[plan9port](https://swtch.com/plan9port/) installed.

However, be aware that there exist some incompatibilities between the
implementation in sam and vis which need to be taken into account
when writing tests:

 * _missing commands_: vis does currently deliberately not implement
   some commands available in sam (e.g. `m`, `t`, `=` `=#`, `k`, `u`).
   Additionally the commands operating on multiple files are either
   not implemented or currently not supported by the testing
   infrastructure.

 * _different semantics_: some language constructs have slightly
   different semantics. Of particular relevance is the different grouping
   implementation. In sam each command of a group is executed with
   the same initial file state and all changes are later applied in parallel
   (if no conflict occurred and all changes were in sequence). Whereas
   vis applies the changes immediately and subsequent commands will
   operate on the modified file content.

   As a consequence the following snippet to swap two words does currently
   not work in vis:

    ```
    ,x/[a-zA-Z]+/{
      g/Emacs/ v/....../ c/vi/
      g/vi/ v/.../ c/Emacs/
    }
    ```

   This is particularly relevant for the tests because they are all
   executed in an implicit group.

 * _changes need to be in sequence_: sam will reject changes which are not
   in sequence (i.e. all changes need to be performed in ascending file
   position). The following will not work:

    ```
    {
      10d
      5d
    }
    ?changes not in sequence
    ```

   However, due to the aforementioned vis grouping semantics, swapping
   the line numbers around, will not produce the same result either
   (line 5 will already be deleted by the time the address of line 10
   is evaluated, thus the original line 11 will be deleted instead).

 * _spurious white spaces_: in sam an empty line/command affects the
   dot (current range to operate on), namely it is extended to cover
   the whole line. However in vis we ignore all white space between
   commands. In order to avoid differing behavior avoid spurious white
   space between your commands in the test cases.

For now the tests need to take these quirks into account, some of the
vis behavior might be changed in the future.

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
