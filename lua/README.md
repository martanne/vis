Tests for vis specific lua api
------------------------------

There are two types of lua tests here:

1. Tests are formed from a `<test>.in`, `<test>.ref` and `<test>.out` triplet.
   The `<test>.in` file is opened by vis, some operatations are performed and
   the modified file is written to `<test>.out`. The new `<test>.out` is
   compared to `<test>.ref` and the test passes if they are identical.

2. Tests are formed from a single `<test>.status` file. This file is created by
   the lua code in the test. It contains a single line per test case, this
   single line should be `true` if the test case passed. The `<test>.status`
   file is checked to ensure it contains only `true` lines and if so, the test
   passes.

The shell script `test.sh` looks for a file with extension `.in`, eg `test.in`,
and opens it in vis. The corresponding lua file, `test.lua`, is executed and is 
expected to create either a `test.out` or `test.status` file.

Type `make` to run all tests.
