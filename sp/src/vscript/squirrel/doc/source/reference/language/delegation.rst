.. _delegation:


========================
Delegation
========================

.. index::
    single: Delegation

Squirrel supports implicit delegation. Every table or userdata can have a parent table
(delegate). A parent table is a normal table that allows the definition of special behaviors
for his child.
When a table (or userdata) is indexed with a key that doesn't correspond to one of its
slots, the interpreter automatically delegates the get (or set) operation to its parent.::

    Entity <- {
    }

    function Entity::DoStuff()
    {
        ::print(_name);
    }

    local newentity = {
        _name="I'm the new entity"
    }
    newentity.setdelegate(Entity)

    newentity.DoStuff(); //prints "I'm the new entity"

The delegate of a table can be retreived through built-in method ``table.getdelegate()``.::

    local thedelegate = newentity.getdelegate();

