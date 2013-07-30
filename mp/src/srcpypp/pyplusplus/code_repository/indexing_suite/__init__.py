# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
code repository for Indexing Suite V2 - std containers wrappers
"""

all = []

from . import slice_header
all.append( slice_header )

from . import set_header
all.append( set_header )

from . import element_proxy_traits_header
all.append( element_proxy_traits_header )

from . import python_iterator_header
all.append( python_iterator_header )

from . import proxy_iterator_header
all.append( proxy_iterator_header )

from . import element_proxy_header
all.append( element_proxy_header )

from . import container_suite_header
all.append( container_suite_header )

from . import slice_handler_header
all.append( slice_handler_header )

from . import workaround_header
all.append( workaround_header )

from . import value_traits_header
all.append( value_traits_header )

from . import visitor_header
all.append( visitor_header )

from . import algorithms_header
all.append( algorithms_header )

from . import vector_header
all.append( vector_header )

from . import methods_header
all.append( methods_header )

from . import deque_header
all.append( deque_header )

from . import shared_proxy_impl_header
all.append( shared_proxy_impl_header )

from . import iterator_range_header
all.append( iterator_range_header )

from . import int_slice_helper_header
all.append( int_slice_helper_header )

from . import container_traits_header
all.append( container_traits_header )

from . import suite_utils_header
all.append( suite_utils_header )

from . import list_header
all.append( list_header )

from . import map_header
all.append( map_header )

from . import iterator_traits_header
all.append( iterator_traits_header )

from . import container_proxy_header
all.append( container_proxy_header )

from . import multimap_header
all.append( multimap_header )

from . import pair_header
all.append( pair_header )

from . import registry_utils_header
all.append( registry_utils_header )

headers = [f.file_name for f in all]