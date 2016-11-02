Testing Infrastructure for Vis
------------------------------

This repository contains testing infrastructure for the
[vis editor](https://github.com/martanne/vis). It is expected
to be cloned into a sub directory of the `vis` source tree.

There exist 5 different kinds of tests:

 * `core` are C unit tests for core data structures used by vis
 * `vim` tests vim compatibility
 * `sam` tests sam compatibility of the command language
 * `vis` contains tests for vis specific behavior/features
 * `lua` contains tests for the vis specific lua api

Run `make` to execute all test suites.
