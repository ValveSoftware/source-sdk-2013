# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

file_name = "ctypes_utils.py"

code = \
"""# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import ctypes

# what is the best way to treat overloaded constructors
class native_callable( object ):
    def __init__(self, dll, name, restype=None, argtypes=None ):
        self.name = name
        self.func = getattr( dll, dll.undecorated_names[name] )
        self.func.restype = restype
        self.func.argtypes = argtypes

    def __call__(self, *args, **keywd ):
        return self.func( *args,  **keywd )

class native_overloaded_callable( object ):
    def __init__(self, functions ):
        self.__functions = functions

    def __call__( self, *args ):
        types = None
        if args:
            types = tuple(arg.__class__ for arg in args)
        f = self.__functions.get(types)
        if f is None:
            msg = ['Unable to find a function that match the arguments you passed.']
            msg.append( 'First argument type is "this" type.' )
            msg.append( 'This function call argument types: ' + str( types ) )
            msg.append( 'Registered methods argument types: ' )
            for key in self.__functions.iterkeys():
                msg.append('    ' + str(key))
            raise TypeError(os.linesep.join(msg))
        else:
            return f(*args)

class multi_method_registry_t:
    def __init__( self, factory, restype ):
        self.factory = factory
        self.restype = restype
        self.__functions = {}

    def register( self, callable_or_name, argtypes=None ):
        if isinstance( callable_or_name, native_callable ):
            callable = callable_or_name
        else:
            name = callable_or_name
            callable = self.factory( name, restype=self.restype, argtypes=argtypes )
        self.__functions[ tuple(callable.func.argtypes) ] = callable.func
        return self

    def finalize(self):
        return native_overloaded_callable( self.__functions )


class mem_fun_factory( object ):
    def __init__( self, dll, wrapper, class_name, namespace='' ):
        self.dll = dll
        self.namespace = namespace
        self.class_name = class_name
        self.this_type = ctypes.POINTER( wrapper )

    def __call__( self, name, **keywd ):
        if 'argtypes' not in keywd or keywd['argtypes'] is None:
            keywd['argtypes'] = [ self.this_type ]
        else:
            keywd['argtypes'].insert( 0, self.this_type )
        return native_callable( self.dll, name, **keywd )

    def __get_ns_name(self):
        if self.namespace:
            return self.namespace + '::'
        else:
            return ''

    def default_constructor( self ):
        return self( '%(ns)s%(class_name)s::%(class_name)s(void)'
                        % dict( ns=self.__get_ns_name()
                                , class_name=self.class_name ) )

    def constructor( self, argtypes_str, **keywd ):
        return self( '%(ns)s%(class_name)s::%(class_name)s(%(args)s)'
                        % dict( ns=self.__get_ns_name()
                                , class_name=self.class_name
                                , args=argtypes_str )
                     , **keywd )

    def copy_constructor( self ):
        return self( '%(ns)s%(class_name)s::%(class_name)s(%(ns)s%(class_name)s const &)'
                        % dict( ns=self.__get_ns_name()
                                , class_name=self.class_name )
                     , argtypes=[self.this_type] )

    def destructor( self, is_virtual=False ):
        virtuality = ''
        if is_virtual:
            virtuality = 'virtual '
        return self( '%(virtuality)s%(ns)s%(class_name)s::~%(class_name)s(void)'
                        % dict( ns=self.__get_ns_name()
                                , virtuality=virtuality
                                , class_name=self.class_name ) )

    def operator_assign( self ):
        return self( '%(ns)s%(class_name)s & %(class_name)s::operator=(%(class_name)s const &)'
                        % dict( ns=self.__get_ns_name()
                                , class_name=self.class_name )
                     , restype=self.this_type
                     , argtypes=[self.this_type] )

    def method( self, name, restype_str=None, argtypes_str=None, **keywd ):
        if None is restype_str:
            restype_str = 'void'
        if None is argtypes_str:
            argtypes_str = 'void'

        return self( '%(return_)s %(ns)s%(class_name)s::%(method_name)s(%(args)s)'
                        % dict( return_=restype_str
                                , ns=self.__get_ns_name()
                                , class_name=self.class_name
                                , method_name=name
                                , args=argtypes_str )
                     , **keywd )

    def multi_method( self, restype=None ):
        return multi_method_registry_t( self, restype )


#take a look on http://code.activestate.com/recipes/413486/
Enumeration = ctypes.c_int
"""
