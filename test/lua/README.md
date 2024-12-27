Unit Test for Vis' Lua API
--------------------------

The test-suite is based on the [busted unit testing framework](https://olivinelabs.com/busted/).

Each `*.lua` file is sourced from a new `vis` instance which loads the
correspending `*.in` file.

Use the `test.sh` shell script to run an individual test or type `make`
to run all tests.
