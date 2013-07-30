# Copyright 2004 Giovanni Beltrame
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines a class that writes :class:`code_creators.bpmodule_t` to multiple files"""

import os
import math
from . import multiple_files
from pyplusplus import messages
from pyplusplus import _logging_
from pygccxml import declarations
from pyplusplus import decl_wrappers
from pyplusplus import code_creators
from pyplusplus.utils import split_sequence

#TODO: to add namespace_alias_t classes
class balanced_files_t(multiple_files.multiple_files_t):
    """
    This class implements classic strategy of dividing classes to files
    one class in one header + source files.
    """
    HEADER_EXT = '.pypp.hpp'
    SOURCE_EXT = '.pypp.cpp'

    def __init__( self
                  , extmodule
                  , directory_path
                  , number_of_buckets
                  , write_main=True
                  , files_sum_repository=None
                  , encoding='ascii'):
        """Constructor.

        :param extmodule: The root of a code creator tree
        :type extmodule: :class:`code_creators.bpmodule_t`
        :param directory_path: The output directory where the source files are written
        :type directory_path: str

        :param write_main:  if it is True, the class will write out a main file
            that calls all the registration methods.
        :type write_main: boolean
        """
        multiple_files.multiple_files_t.__init__( self, extmodule, directory_path, write_main, files_sum_repository, encoding)
        self.number_of_buckets = number_of_buckets

    def split_classes( self ):
        class_creators = [x for x in self.extmodule.body.creators if isinstance(x, ( code_creators.class_t, code_creators.class_declaration_t ) )]

        class_creators = [cc for cc in class_creators if not cc.declaration.already_exposed]

        buckets = split_sequence(class_creators, len(class_creators)/self.number_of_buckets )
        if len(buckets) > self.number_of_buckets:
            buckets[len(buckets)-2] += buckets[len(buckets)-1]
            buckets = buckets[:len(buckets)-1]

        for index, bucket in enumerate( buckets ):
            self.split_creators( bucket
                                 , '_classes_%d' % (index+1)
                                 , 'register_classes_%d' % (index+1)
                                 , -1 )
