Tests for vis - a vim-like editor frontend
------------------------------------------

The basic idea is to feed the same keyboard input to both vim
and vis and compare their respective results.

A test constitutes of 2 files:

 * `test.in` the file/buffer content with which the editor is started
 * `test.keys` a file containing the keyboard input as would normally
   be typed by a user

The `*.keys` file are piped through the C preprocessor `cpp(1)`, trailing
white spaces are stripped before the [keys utility](../util) translates
the symbolic keys to terminal input sequences suitable for consumption
by the actual editor processes. In rare cases it might be necessary to
inspect intermediate states of this pipeline for debugging purposes.
Some general hints on writing test:

 * Make sure all input files are properly new line terminated, otherwise
   you will get test failures because vim will add a trailing new line
   whereas vis leafs the input file as is.

 * Place logically different commands on different input lines. The
   new lines are insignificant. To enter a literal new line use `<Enter>`.

 * Use C style comments `/* ... */` or `// ...` to explain complex text
   manipulations. However, be aware that due to the use of the C
   preprocessor comments do not mix well with unterminated string
   constants as used to specify registers.

The top level shell script `test.sh` looks for the `*.keys` files in
subdirectories, executes both editors and compares the resulting output.
`test.sh path/to/test` (without the file extension) runs an individual
test.

Type `make` to run all tests.
