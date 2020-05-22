.. _stdlib_stdmathlib:

================
The Math library
================

the math lib provides basic mathematic routines. The library mimics the
C runtime library implementation.

------------
Squirrel API
------------

+++++++++++++++
Global Symbols
+++++++++++++++

.. js:function:: abs(x)

    returns the absolute value of `x` as an integer

.. js:function:: acos(x)

    returns the arccosine of `x`

.. js:function:: asin(x)

    returns the arcsine of `x`

.. js:function:: atan(x)

    returns the arctangent of `x`

.. js:function:: atan2(x,y)

    returns the arctangent of  `x/y`

.. js:function:: ceil(x)

    returns a float value representing the smallest integer that is greater than or equal to `x`

.. js:function:: cos(x)

    returns the cosine of `x`

.. js:function:: exp(x)

    returns the exponential value of the float parameter `x`

.. js:function:: fabs(x)

    returns the absolute value of `x` as a float

.. js:function:: floor(x)

    returns a float value representing the largest integer that is less than or equal to `x`

.. js:function:: log(x)

    returns the natural logarithm of `x`

.. js:function:: log10(x)

    returns the logarithm base-10 of `x`

.. js:function:: pow(x,y)

    returns `x` raised to the power of `y`

.. js:function:: rand()

    returns a pseudorandom integer in the range 0 to `RAND_MAX`

.. js:function:: sin(x)

    rreturns the sine of `x`

.. js:function:: sqrt(x)

    returns the square root of `x`

.. js:function:: srand(seed)

    sets the starting point for generating a series of pseudorandom integers

.. js:function:: tan(x)

    returns the tangent of `x`

.. js:data:: RAND_MAX

    the maximum value that can be returned by the `rand()` function

.. js:data:: PI

    The numeric constant pi (3.141592) is the ratio of the circumference of a circle to its diameter

------------
C API
------------

.. _sqstd_register_mathlib:

.. c:function:: SQRESULT sqstd_register_mathlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initializes and register the math library in the given VM.

