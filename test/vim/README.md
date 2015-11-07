Tests for vis - a vim-like editor frontend
------------------------------------------

The basic idea is to feed the same keyboard input to both vim
and vis and compare their respective results.

A test constitutes of 2 files:

 * `test.in` the file/buffer content with which the editor is started
 * `test.keys` a file containing the keyboard input as would normally
   be typed by a user

The toplevel shell script `test.sh` looks for these files in subdirectories,
executes both editors and compares the resulting output.

Type `make` to run all tests.
