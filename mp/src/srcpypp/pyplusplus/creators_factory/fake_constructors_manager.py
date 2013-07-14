# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines class, which manages "fake constructors" """

class manager_t( object ):
    def __init__( self, global_ns ):
        self.__global_ns = global_ns
        self.__fc_all = set()
        self.__fc_exposed = set()

        for cls in global_ns.classes(recursive=True, allow_empty=True):
            for fc in cls.fake_constructors:
                self.__fc_all.add( fc )
                if fc.ignore:
                    continue
                if not fc.exportable:
                    continue
                if not isinstance( fc, cls.FAKE_CONSTRUCTOR_TYPES ):
                    continue
                self.__fc_exposed.add( fc )

    def is_fake_constructor( self, fc ):
        return fc in self.__fc_all

    def should_generate_code( self, fc ):
        return fc in self.__fc_exposed
