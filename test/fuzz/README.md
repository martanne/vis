Fuzzing infrastructure for low level code used by vis
-----------------------------------------------------

This directory contains some simple command line applications
which expose core library interfaces through the standard I/O
streams. They are intended to be used as test drivers for
fuzzers like [AFL](http://lcamtuf.coredump.cx/afl/).

Run one of the `make afl-fuzz-*` targets to start fuzzing a
specific instrumented binary using `afl-fuzz(1)`. By default
it will try to resume a previous fuzzing session, before
starting a new one if that fails.

The following files are used:

 * `$APP-fuzzer.c` application exposing a simple text interface
 * `fuzzer.h` common code used among different fuzzing drivers
 * `./input/$APP/` intial test input, one file per test
 * `./dictionaries/$APP.dict` a dictionary with valid syntax tokens
 * `./results/$APP/` the fuzzing results are stored here

See the AFL documentation for further information.

In the future we might also use [libFuzzer](http://llvm.org/docs/LibFuzzer.html)
for further fuzzing.

Quick start example:

    $ make afl-fuzz-text

