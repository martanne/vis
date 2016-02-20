Tests for vis specific editing features
---------------------------------------

The basic idea is to feed keyboard input to `vis` and compare the
produced output with a known reference solution.

A test constitutes of 3 files:

 * `test.in` the file/buffer content with which the editor is started
 * `test.keys` a file containing the keyboard input as would normally
   be typed by a user
 * `test.ref` a reference file at the end of the editing session

The toplevel shell script `test.sh` looks for these files in sub
directories, feeds the keys to `vis` and compares the produces output.

Type `make` to run all tests.
