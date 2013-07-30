# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines documentation extractor class interface"""

import re

class doc_extractor_i(object):
    """defines documentation extractor interface"""

    __escape_re = re.compile (r'((\\x[a-f0-9][a-f0-9])|(\\*"))', re.I)

    def __init__( self, encoding='ascii' ):
        object.__init__( self )
        self.encoding = encoding

    def extract( self, decl ):
        """returns documentation text for the declaration

        This function should be implemented in derived class.

        Using decl.location.file_name and decl.location.line variables you can
        find out the location of declaration within source file
        """
        raise NotImplementedError()

    def __call__( self, decl ):
        decl_doc = self.extract( decl )
        return self.escape_doc( decl_doc )

    @staticmethod
    def escape_doc( doc ):
        """converts a text to be valid C++ string literals"""
        def replace_escape(m):
            g = m.group(1)
            if g.startswith ('\\x'):
                return g + '""'
            elif g == '"':
                return '\\"'
            else:
                return g
        return '"%s"' % doc_extractor_i.__escape_re.sub( replace_escape, repr(doc)[1:-1] )

