Text
====

The core text management data structure which supports efficient
modifications and provides a byte string interface. Text positions
are represented as ``size_t``.  Valid addresses are in range ``[0,
text_size(txt)]``. An invalid position is denoted by ``EPOS``. Access to
the non-contigiuos pieces is available by means of an iterator interface
or a copy mechanism. Text revisions are tracked in an history graph.

.. note:: The text is assumed to be encoded in `UTF-8 <https://tools.ietf.org/html/rfc3629>`_.

Load
----

.. doxygengroup:: load
   :content-only:

State
-----

.. doxygengroup:: state
   :content-only:

Modify
------

.. doxygengroup:: modify
   :content-only:

Access
------

The individual pieces of the text are not necessarily stored in a
contiguous memory block. These functions perform a copy to such a region.

.. doxygengroup:: access
   :content-only:

Iterator
--------

An iterator points to a given text position and provides interfaces to
adjust said position or read the underlying byte value. Functions which
take a ``char`` pointer will generally assign the byte value *after*
the iterator was updated.

.. doxygenstruct:: Iterator

.. doxygengroup:: iterator
   :content-only:

Byte
^^^^

.. note:: For a read attempt at EOF (i.e. `text_size`) an artificial ``NUL``
          byte which is not actually part of the file is returned.

.. doxygengroup:: iterator_byte
   :content-only:

Codepoint
^^^^^^^^^

These functions advance to the next/previous leading byte of an UTF-8
encoded Unicode codepoint by skipping over all continuation bytes of
the form ``10xxxxxx``.

.. doxygengroup:: iterator_code
   :content-only:

Grapheme Clusters
^^^^^^^^^^^^^^^^^

These functions advance to the next/previous grapheme cluster. 

.. note:: The grapheme cluster boundaries are currently not implemented
          according to `UAX#29 rules <http://unicode.org/reports/tr29>`_.
          Instead a base character followed by arbitrarily many combining
          character as reported by ``wcwidth(3)`` are skipped.

.. doxygengroup:: iterator_char
   :content-only:

Lines
-----

Translate between 1 based line numbers and 0 based byte offsets.

.. doxygengroup:: lines
   :content-only:

History
-------

Interfaces to the history graph.

.. doxygengroup:: history
   :content-only:

Marks
-----

A mark keeps track of a text position. Subsequent text changes will update
all marks placed after the modification point. Reverting to an older text
state will hide all affected marks, redoing the changes will restore them.

.. warning:: Due to an optimization cached modifications (i.e. no ``text_snaphot``
             was performed between setting the mark and issuing the changes) might
             not adjust mark positions accurately.

.. doxygentypedef:: Mark

.. doxygendefine:: EMARK

.. doxygengroup:: mark
   :content-only:

Save
----

.. doxygengroup:: save
   :content-only:

Miscellaneous
-------------

.. doxygengroup:: misc
   :content-only:
