Tests for vis specific lua api
------------------------------

Tests are formed from a `<test>.in`, `<test>.ref` and `<test>.out` triplet.  
The `<test>.in` file is opened by vis, some operatations are performed and the 
modified file is written to `<test>.out`. The new `<test>.out` is compared to 
`<test>.ref` and the test passes if they are identical.

The shell script `test.sh` looks for a file with extension `.in`, eg `test.in`,
and opens it in vis. The corresponding lua file, `test.lua`, is executed and is 
expected to create a `test.out` file.

Type `make` to run all tests.
