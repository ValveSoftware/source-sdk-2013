# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines interface for exposing STD containers, using current version of indexing suite"""

from pygccxml import declarations
from . import python_traits
#NoProxy
#By default indexed elements have Python reference semantics and are returned by
#proxy. This can be disabled by supplying true in the NoProxy template parameter.
#We want to disable NoProxy when we deal with immutable objects.

containers = {
    'vector' : "boost/python/suite/indexing/vector_indexing_suite.hpp"
    , 'map' : "boost/python/suite/indexing/map_indexing_suite.hpp"
}


class indexing_suite1_t( object ):
    """
    This class helps user to export STD containers, using built-in Boost.Python
    indexing suite.
    """

    def __init__( self, container_class, no_proxy=None, derived_policies=None ):
        object.__init__( self )
        self.__no_proxy = no_proxy
        self.__derived_policies = derived_policies
        self.__container_class = container_class
        self.__include_files = None
        self.__element_type = None

    @property
    def container_class( self ):
        """reference to the parent( STD container ) class"""
        return self.__container_class

    @property
    def element_type(self):
        """reference to container value_type( mapped_type ) type"""
        if self.__element_type is None:
            self.__element_type = self.container_class.container_traits.element_type( self.container_class )
        return self.__element_type
        
    @property
    def container_traits( self ):
        "reference to container traits. See pygccxml documentation for more information."
        return self.container_class.container_traits
    
    def _get_no_proxy( self ):
        if self.__no_proxy is None:
            self.__no_proxy = python_traits.is_immutable( self.element_type )
        return self.__no_proxy

    def _set_no_proxy( self, no_proxy ):
        self.__no_proxy = no_proxy
    no_proxy = property( _get_no_proxy, _set_no_proxy
                         , doc="NoProxy value, the initial value depends on container"
                              +" element_type( mapped_type ) type. In most cases, "
                              +"`Py++` is able to guess this value, right. If you are not "
                              +"lucky, you will have to set the property value.")

    def _get_derived_policies( self ):
        return self.__derived_policies
    def _set_derived_policies( self, derived_policies ):
        self.__derived_policies = derived_policies
    derived_policies = property( _get_derived_policies, _set_derived_policies
                                 , doc="This proprty contains DerivedPolicies string. "
                                      +"It will be added as is to the generated code.")

    @property
    def include_files( self ):
        """Return list of header files to be included in generated code"""
        if self.__include_files is None:
            name = self.container_class.name.split( '<' )[0]
            if name not in containers:
                self.__include_files = [] #not supported
            else:
                self.__include_files = [containers[ name ]]
        return self.__include_files
