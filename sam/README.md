Tests for vis - structural regular expression support
-----------------------------------------------------

We test the structural regular expression implementation by supplying
the same command to both vis and [sam](http://sam.cat-v.org/). More
concretely, we use a `ssam(1)` like script drive the non-graphical
streaming interface of sam.

These tests are intended to be run on a system with
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
   different semantics. As an example in `sam` searches wrap around
   while in `vis` they stop at the start/end of the file.

 * _changes need to be in sequence_: sam will reject changes which are not
   in sequence (i.e. all changes need to be performed in monotonically
   increasing file position). The following will not work:

    ```
    ,x/pattern/ {
        a/</
        i/>/
    }
    ?changes not in sequence
    ```

   In contrast vis only requires that the changes are non-overlapping.
   The above works as expected, but the following is rejected:

    ```
    ,x/pattern/ {
        c/foo/
        c/bar/
    }
    ?changes not in sequence
    ```

 * _spurious white spaces_: in sam an empty line/command affects the
   dot (current range to operate on), namely it is extended to cover
   the whole line. However in vis we ignore all white space between
   commands. In order to avoid differing behavior avoid spurious white
   space between your commands in the test cases.

For now the tests need to take these differences into account, some of the
vis behavior might be changed in the future.

A test constitutes of 3 files, the first 2 of which are mandatory:

 * `test.in` the file/buffer content with which the editor is started
 * `test.cmd` a file containing the structural regular expression
    command as you would type it in vis at the `:`-prompt or pass to
    `ssam(1)`.
 * `test.ref` if omitted, the output from sam will be considered as
    reference. 

All commands of a `test.cmd` are executed in an implicit group `.{ ... }`.

The top level shell script `test.sh` looks for these files in sub
directories, executes both editors and compares the resulting output.

Type `make` to run all tests.
