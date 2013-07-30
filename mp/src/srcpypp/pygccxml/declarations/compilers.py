# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
contains enumeration of all compilers supported by the project
"""

GCC_XML_06 = "GCC-XML 0.6"
GCC_XML_07 = "GCC-XML 0.7"
GCC_XML_09 = "GCC-XML 0.9"
GCC_XML_09_BUGGY = "GCC-XML 0.9 BUGGY"
#revision 122:
#After this fix, all constructors and destructors that exist for a class
#are dumped whether the user declared them or not.  Those that were
#implicitly declared by the compiler are marked as "artificial".

def on_missing_functionality( compiler, functionality ):
    raise  NotImplementedError( '"%s" compiler doesn\'t support functionality "%s"'
                                % ( compiler, functionality ))
