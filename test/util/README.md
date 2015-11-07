Utility to turn symbolic keys into terminal input
-------------------------------------------------

This is a small helper utility which translates symbolic keys, as provided
by [libtermkey](http://www.leonerd.org.uk/code/libtermkey/) and also used
to specify key bindings within vis, to their corresponding codes understood
by terminal programs.

Type `make` to build the utility. `make keys-local` links the utility against
a locally built version of libtermkey as produced by the top level `make local`
Makefile target.
