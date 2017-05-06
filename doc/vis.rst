Vis
===

The core Vis API.

Lifecycle
---------

.. doxygengroup:: vis_lifecycle
   :content-only:

Draw
----

.. doxygengroup:: vis_draw
   :content-only:

Windows
-------

.. doxygengroup:: vis_windows
   :content-only:

Input
-----

The editor core processes input through a sequences of symbolic keys:

 * Special keys such as ``<Enter>``, ``<Tab>`` or ``<Backspace>`` as reported by
   `termkey_strfkey <http://www.leonerd.org.uk/code/libtermkey/doc/termkey_strfkey.3.html>`_.

   .. note:: The prefixes ``C-``, ``S-`` and ``M-`` are used to denote the ``Ctrl``,
             ``Shift`` and ``Alt`` modifiers, respectively.

 * Key action names as registered with `vis_action_register`.

   .. note:: By convention they are prefixed with ``vis-`` as in ``<vis-nop>``.

 * Regular UTF-8 encoded input.

.. note:: An exhaustive list of the first two types is displayed in the ``:help`` output.

.. doxygengroup:: vis_keys
   :content-only:

Key Map
-------

The key map is used to translate keys in non-input modes, *before* any key
bindings are evaluated. It is intended to facilitate usage of non-latin keyboard
layouts.

.. doxygengroup:: vis_keymap
   :content-only:

Key Binding
-----------

Each mode has a set of key bindings. A key binding maps a key to either
another key (referred to as an alias) or a key action (implementing an
editor operation).

If a key sequence is ambiguous (i.e. it is a prefix of multiple mappings)
more input is awaited, until a unique mapping can be resolved.

.. warning:: Key aliases are always evaluated recursively.

.. doxygengroup:: vis_keybind
   :content-only:

Key Action
----------

A key action is invoked by a key binding and implements a certain editor function.

The editor operates like a finite state machine with key sequences as
transition labels. Once a prefix of the input queue uniquely refers to a
key action, it is invoked with the remainder of the input queue passed as argument.

.. note:: A triggered key action currently does not know through which key binding
          it was invoked. TODO: change that?

.. doxygengroup:: vis_action
   :content-only:

Modes
-----

A mode defines *enter*, *leave* and *idle* actions and captures a set of
key bindings.

Modes are hierarchical, key bindings are searched recursively towards
the top of the hierarchy stopping at the first match.

.. doxygenenum:: VisMode
.. doxygengroup:: vis_modes
   :content-only:

Count
-----

Dictates how many times a motion or text object is evaluated. If none
is specified, a minimal count of 1 is assumed.

.. doxygengroup:: vis_count
   :content-only:

Operators
---------

.. doxygengroup:: vis_operators
   :content-only:

Motions
-------

.. doxygengroup:: vis_motions
   :content-only:

Text Objects
------------

.. doxygengroup:: vis_textobjs
   :content-only:

Marks
-----

Marks keep track of a given text position.

.. note:: Marks are currently file local.

.. doxygengroup:: vis_marks
   :content-only:

Registers
---------

.. doxygengroup:: vis_registers
   :content-only:

Macros
------

Macros are a sequence of keys stored in a Register which can be reprocessed
as if entered by the user.

.. warning:: Macro support is currently half-baked. If you do something stupid
             (e.g. use mutually recursive macros), you will likely encounter
             stack overflows.

.. doxygengroup:: vis_macros
   :content-only:

Commands
--------

.. doxygengroup:: vis_cmds
   :content-only:

Options
-------

.. doxygengroup:: vis_options
   :content-only:

Modification
------------

These function operate on the currently focused window but ensure that
all windows which show the affected region are redrawn too.

.. doxygengroup:: vis_changes
   :content-only:

Interaction
-----------

.. doxygengroup:: vis_info
   :content-only:

Miscellaneous
-------------

.. doxygengroup:: vis_misc
   :content-only:

