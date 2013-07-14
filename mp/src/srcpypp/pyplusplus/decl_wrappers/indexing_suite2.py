# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines interface for exposing STD containers, using next version of indexing suite"""

from pygccxml import declarations
from . import call_policies
"""
method_len
method_iter
method_getitem
method_getitem_slice
method_index
method_contains
method_count
method_has_key
method_setitem
method_setitem_slice
method_delitem
method_delitem_slice
method_reverse
method_append
method_insert
method_extend
method_sort

slice_methods = method_getitem_slice | method_setitem_slice | method_delitem_slice
search_methods = method_index | method_contains | method_count | method_has_key
reorder_methods = method_sort | method_reverse
insert_methods = method_append | method_insert | method_extend
"""


containers = {
      'vector' : "indexing_suite/vector.hpp"
      , 'deque' : "indexing_suite/deque.hpp"
      , 'list' : "indexing_suite/list.hpp"
      , 'map' : "indexing_suite/map.hpp"
      , 'multimap' : "indexing_suite/multimap.hpp"
      , 'hash_map' : "indexing_suite/map.hpp"
      #, 'hash_multimap' : "indexing_suite/multimap.hpp"
      , 'set' : "indexing_suite/set.hpp"
      , 'hash_set' : "indexing_suite/set.hpp"
    #TODO: queue, priority, stack, multiset, hash_multiset
}

class indexing_suite2_t( object ):
    """
    This class helps user to export STD containers, using Boost.Python
    indexing suite V2.
    """

    #List of method names. These method could be excluded from being exposed.
    METHODS = ( 'len', 'iter', 'getitem', 'getitem_slice', 'index', 'contains'
                , 'count', 'has_key', 'setitem', 'setitem_slice', 'delitem'
                , 'delitem_slice', 'reverse', 'append', 'insert', 'extend', 'sort' )

    #Dictionary of method group names. These method groups could be excluded from
    #being exposed. Dictionary key is a method group name. Dictionary value is a
    #list of all methods, which belong to the group.
    METHOD_GROUPS = {
        'slice' : ( 'method_getitem_slice', 'method_setitem_slice', 'method_delitem_slice' )
        , 'search' : ( 'method_index', 'method_contains', 'method_count', 'method_has_key' )
        , 'reorder' : ( 'method_sort', 'method_reverse' )
        , 'insert' : ( 'method_append', 'method_insert', 'method_extend' )
    }

    def __init__( self, container_class ):
        object.__init__( self )
        self.__call_policies = None
        self.__container_class = container_class
        self._disabled_methods = set()
        self._disabled_groups = set()
        self._default_applied = False
        self._use_container_suite = False
        self.__include_files = None

    def get_use_container_suite( self ):
        return self._use_container_suite
    def set_use_container_suite( self, value ):
        self._use_container_suite = value
    use_container_suite = property( get_use_container_suite, set_use_container_suite )

    @property
    def container_class( self ):
        """reference to the parent( STD container ) class"""
        return self.__container_class

    @property
    def element_type(self):
        """reference to container value_type( mapped_type ) type"""
        return self.container_traits.element_type( self.container_class )

    @property
    def container_traits( self ):
        "reference to container traits. See pygccxml documentation for more information."
        return self.container_class.container_traits

    def _get_call_policies( self ):
        if self.__call_policies:
            return self.__call_policies
        if self.container_traits not in declarations.sequential_container_traits:
            #TODO: find out why map's don't like the policy
            return self.__call_policies
        element_type = None
        try:
            element_type = self.element_type
        except:
            return
        if declarations.is_const( element_type ):
            element_type = declarations.remove_const( element_type )
        if declarations.is_pointer( element_type ):
            self.__call_policies = call_policies.return_internal_reference()
        return self.__call_policies

    def _set_call_policies( self, call_policies ):
        self.__call_policies = call_policies
    call_policies = property( _get_call_policies, _set_call_policies
                              , "Call policies, that should be used by Boost.Python container classes.")

    def __apply_defaults_if_needed( self ):
        if self._default_applied:
            return
        self._default_applied = True
        #find out what operators are supported by element_type and
        #then configure the _disable_[methods|groups]
        pass

    def disable_method( self, method_name ):
        """Disable method from being exposed"""
        assert method_name in self.METHODS
        self.__apply_defaults_if_needed()
        self._disabled_methods.add( method_name )

    def enable_method( self, method_name ):
        """Enable method to be exposed"""
        assert method_name in self.METHODS
        self.__apply_defaults_if_needed()
        if method_name in self._disabled_methods:
            self._disabled_methods.remove( method_name )

    def _get_disabled_methods( self ):
        self.__apply_defaults_if_needed()
        return self._disabled_methods
    disable_methods = property( _get_disabled_methods
                                , doc="list of all disabled methods")

    def disable_methods_group( self, group_name ):
        """Disable methods group from being exposed"""
        assert group_name in self.METHOD_GROUPS
        self.__apply_defaults_if_needed()
        self._disabled_groups.add( group_name )

    def enable_methods_group( self, group_name ):
        """Enable methods group to be exposed"""
        assert group_name in self.METHOD_GROUPS
        self.__apply_defaults_if_needed()
        if group_name in self._disabled_groups:
            self._disabled_groups.remove( group_name )

    def _get_disabled_methods_groups( self ):
        self.__apply_defaults_if_needed()
        return self._disabled_groups
    disabled_methods_groups = property( _get_disabled_methods_groups
                                        , doc="list of all disabled methods group")

    @property
    def include_files( self ):
        """Return list of header files to be included in generated code"""
        if self.__include_files is None:
            name = self.container_class.name.split( '<' )[0]
            if name not in containers:
                self.__include_files = [] #not supported
            else:
                #impl details: the order of header files is IMPORTANT
                self.__include_files = [ "indexing_suite/container_suite.hpp"
                                         , containers[ name ] ]
        return self.__include_files
