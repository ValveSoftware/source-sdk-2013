# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
Code repository package is used as a repository of C++/Python classes/functions.
Those classes/functions solve problems, that are typical to most projects.
"""

from . import array_1
from . import gil_guard
from . import named_tuple
from . import convenience
from . import return_range
from . import ctypes_utils
from . import call_policies
from . import indexing_suite
from . import ctypes_integration

all = [ array_1
        , gil_guard
        , convenience
        , call_policies
        , named_tuple
        , return_range
        , ctypes_utils
        , ctypes_integration ]

all.extend( indexing_suite.all )

headers = [f.file_name for f in all]

def i_depend_on_them( fname ):
    """returns list of files, the file fname depends on"""
    if fname in indexing_suite.headers:
        result = indexing_suite.all[:]
        del result[ indexing_suite.headers.index( fname ) ]
        return result
    elif fname == return_range.file_name:
        return indexing_suite.all[:]
    else:
        return []
