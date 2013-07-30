# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import code_creator

class compound_t(code_creator.code_creator_t):
    def __init__(self ):
        """Constructor.

        :param parent: Parent code creator.
        :type parent: :class:`code_creators.code_creator_t`
        """
        code_creator.code_creator_t.__init__( self )
        self._creators = []

    def _get_creators(self):
        return self._creators
    creators = property(_get_creators,
                        doc="""A list of children nodes.
                        @type: list of :class:`code_creators.code_creator_t`""")

    def adopt_creator( self, creator, index=None):
        """Add a creator to the list of children creators.

        :param creator: Creator object
        :type creator: :class:`code_creators.code_creator_t`
        :param index: Desired position of the creator or None to append it to the end of the list
        :type index: int
        """
        creator.parent = self
        if index or index == 0:
            self._creators.insert( index, creator )
        else:
            self._creators.append( creator )

    def adopt_creators( self, creators, index=None):
        """Add a creators to the list of children creators.

        :param creators: list of creators object
        :type creator: :class:`code_creators.code_creator_t`
        :param index: Desired position of the creator or None to append it to the end of the list
        :type index: int
        """
        for pos, creator in enumerate( creators ):
            if index or index == 0:
                self.adopt_creator( creator, index + pos )
            else:
                self.adopt_creator( creator )

    def remove_creator( self, creator ):
        """Remove a children code creator object.

        @precondition: creator must be a children of self
        :param creator: The creator node to remove
        :type creator: :class:`code_creators.code_creator_t`
        """
        creator.parent = None
        del self._creators[ self._creators.index( creator ) ]

    @staticmethod
    def create_internal_code( creators, indent_code=True ):
        """
        concatenate the code from a list of code creators.

        :param creators: A list with code creators
        :type creators: list of :class:`code_creators.code_creator_t`
        :rtype: str
        """
        internals = [expr.create() for expr in creators]
        internals = [_f for _f in internals if _f]
        if indent_code:
            internals = [code_creator.code_creator_t.indent( code ) for code in internals]
        for index in range( len( internals ) - 1):
            internals[index] = internals[index] + os.linesep
        return os.linesep.join( internals )

    def get_system_files( self, recursive=False, unique=False, language='any' ):
        files = super( compound_t, self ).get_system_files(recursive, unique=False, language=language)
        if recursive:
            for creator in self._creators:
                files.extend( creator.get_system_files(recursive, unique=False, language=language) )
        files = [_f for _f in files if _f]
        if unique:
            files = self.unique_headers( files )
        return files

    def find_by_creator_class( self, class_, unique=True, recursive=False ):
        #I will add missing functionality later
        assert recursive == False
        found = [cc for cc in self.creators if isinstance( cc, class_ )]
        if not found:
            return None
        elif 1 == len( found ):
            return found
        else:
            if unique:
                raise LookupError( "Too many creators(%d) of type %s were found."
                                   % ( len( found ), class_.__name__ ) )
            else:
                return found
