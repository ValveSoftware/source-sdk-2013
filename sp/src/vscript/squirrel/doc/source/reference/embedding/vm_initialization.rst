.. _embedding_vm_initialization:

==============================
Virtual Machine Initialization
==============================

The first thing that a host application has to do, is create a virtual machine.
The host application can create any number of virtual machines through the function
*sq_open()*.
Every single VM that was created using *sq_open()* has to be released with the function *sq_close()* when it is no
longer needed.::

    int main(int argc, char* argv[])
    {
        HSQUIRRELVM v;
        v = sq_open(1024); //creates a VM with initial stack size 1024

        //do some stuff with squirrel here

        sq_close(v);
    }
