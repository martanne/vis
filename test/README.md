Testing Infrastructure for Vis
------------------------------

This repository contains testing infrastructure for the
[vis editor](https://github.com/martanne/vis).

There exist 5 different kinds of tests:

 * `core` are C unit tests for core data structures used by vis
 * `fuzz` infrastructure for automated fuzzing
 * `vim` tests vim compatibility
 * `sam` tests sam compatibility of the command language
 * `vis` contains tests for vis specific behavior/features
 * `lua` contains tests for the vis specific lua api

Run `make` to execute all test suites.

Writing good tests
------------------

Each sub directory contains a README with further information
about the specific test method.

Coming up with good and exhaustive tests is often non-trivial,
below are some recommendations:

 * Make sure you understand what the expected behavior you
   want to test is. Think about possible error conditions.

 * Test something specific, but keep the overall context in mind.

   For vi(m) frontend related tests consider behavior when given
   a count or when the command is repeated (using `.`).

   For example the motions `f`, `F`, `t`, `T` also influence `;` and `,`.
   Similar, `*` and `#` affect `n` and `N`.

 * Test special cases, these often reveal interesting behavior.

   Continuing the motion example these might be: empty lines, single
   letter words, no matches, consecutive matches, over-specified counts,
   etc.

 * Test systematically and strive for high test coverage.

   It is preferable to have a small set of tests which cover a specific
   functionality exhaustively, rather than lots of half-baked tests which
   only test the basics.

   A good first indication of the completeness of your tests is the
   [achieved code coverage](https://codecov.io/gh/martanne/vis). Ideally
   a test should primarily exercise a small set of functions which should
   achieve high path coverage.
