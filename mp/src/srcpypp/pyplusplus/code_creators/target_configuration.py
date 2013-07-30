# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

class target_configuration_t( object ):
    """
    Developers do not always work with latest version of boost.python or latest
    version of compiler. So there is a need to generate code that configured to 
    specific version, feature or compiler. Instance of this class will be shared
    between all instances of code creators. Every relevant code creator will 
    respect relevant configuration settings.    
    """
    def __init__( self ):
        self._boost_python_has_wrapper_held_type = True
        self._boost_python_supports_void_ptr = True
        
    def _get_boost_python_has_wrapper_held_type( self ):
        return self._boost_python_has_wrapper_held_type    
    def _set_boost_python_has_wrapper_held_type( self, value ):
        self._boost_python_has_wrapper_held_type = value
    boost_python_has_wrapper_held_type = property( _get_boost_python_has_wrapper_held_type
                                                   , _set_boost_python_has_wrapper_held_type )
    
    def _get_boost_python_supports_void_ptr( self ):
        return self._boost_python_supports_void_ptr    
    def _set_boost_python_supports_void_ptr( self, value ):
        self._boost_python_supports_void_ptr = value
    boost_python_supports_void_ptr = property( _get_boost_python_supports_void_ptr
                                               , _set_boost_python_supports_void_ptr )


    