# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""Python GCC-XML front end.

This package provides functionality to extract and inspect
declarations from C/C++ header files. This is accomplished
by invoking the external tool `gccxml <http://www.gccxml.org/>`_
which parses a header file and dumps the declarations as a
XML file. This XML file is then read by pygccxml and the contents
are made available as appropriate Python objects.

To parse a set of C/C++ header files you use the :func:`parse <parser.parse>`
function in the :mod:parser sub package which returns a tree that contains all
declarations found in the header files. The root of the tree represents the main
namespace `::` and the children nodes represent the namespace contents such as other
namespaces, classes, functions, etc. Each node in the tree is an object of a type
derived from the :class:`declaration_t` class. An inner node is always either a
namespace :class:`declarations.namespace_t` or a class :class:`declarations.class_t`,
which are both derived from :class:`declarations.scopedef_t` class. Everything
else (free functions, member functions, enumerations, variables, etc.) is always
a leaf. You will find all those declaration classes in the :mod:declarations
sub-package.

"""

from . import declarations
from . import parser
from . import utils

#TODO:
#  1. Add "explicit" property for constructors

__version__ = '1.5.1'

__revision__ = 1080
