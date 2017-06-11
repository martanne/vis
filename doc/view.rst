View
====

Provides a viewport of a text instance and mangages selections.

Lifecycle
---------

.. doxygengroup:: view_life
   :content-only:

Viewport
--------

The cursor of the primary selection is always visible.

.. doxygengroup:: view_viewport
   :content-only:

Dimension
---------

.. doxygengroup:: view_size
   :content-only:

Draw
----

.. doxygengroup:: view_draw
   :content-only:

Selections
----------

A selection is a non-empty, directed range with two endpoints called *cursor*
and *anchor*. A selection can be anchored in which case the anchor remains
fixed while only the position of the cursor is adjusted. For non-anchored
selections both endpoints are updated. A singleton selection
covers one character on which both cursor and anchor reside. There always
exists a primary selection which remains visible (i.e. changes to its position
will adjust the viewport).

Creation and Destruction
~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygengroup:: view_selnew
   :content-only:

Navigation
~~~~~~~~~~

.. doxygengroup:: view_navigate
   :content-only:

Cover
~~~~~

.. doxygengroup:: view_cover
   :content-only:

Anchor
~~~~~~

.. doxygengroup:: view_anchor
   :content-only:

Cursor
~~~~~~

Selection endpoint to which cursor motions apply.

Properties
^^^^^^^^^^

.. doxygengroup:: view_props
   :content-only:

Placement
^^^^^^^^^

.. doxygengroup:: view_place
   :content-only:

Motions
^^^^^^^^

These functions perform motions based on the current selection cursor position.

.. doxygengroup:: view_motions
   :content-only:

Primary Selection
~~~~~~~~~~~~~~~~~

These are convenience function which operate on the primary selection.

.. doxygengroup:: view_primary
   :content-only:

Save and Restore
~~~~~~~~~~~~~~~~

.. doxygengroup:: view_save
   :content-only:

Style
-----

.. doxygengroup:: view_style
   :content-only:
