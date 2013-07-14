# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


import os
from . import algorithm
from . import registration_based
from pyplusplus import code_repository
from pyplusplus.decl_wrappers import call_policies
from pyplusplus.decl_wrappers import python_traits
from pygccxml import declarations

class array_1_registrator_t( registration_based.registration_based_t ):
    """
    This class creates code that register static sized array
    """
    def __init__( self, array_type ):
        registration_based.registration_based_t.__init__( self )
        self._array_type = array_type
        self._call_policies = self._guess_call_policies()
        self.works_on_instance = False

    def _get_array_type( self ):
        return self._array_type
    def _set_array_type( self, new_array_type ):
        self._array_type = new_array_type
    array_type = property( _get_array_type, _set_array_type )

    def _get_call_policies( self ):
        return self._call_policies
    def _set_call_policies( self, new_call_policies ):
        self._call_policies = new_call_policies
    call_policies = property( _get_call_policies, _set_call_policies )

    def _create_name( self ):
        item_type = declarations.array_item_type(self.array_type)
        return "__array_1_%(type)s_%(size)d" \
               % dict( type=algorithm.create_valid_name( item_type.decl_string )
                       , size=declarations.array_size(self.array_type) )

    def _guess_call_policies(self):
        item_type = declarations.array_item_type( self.array_type )
        if python_traits.is_immutable( item_type ):
            return call_policies.default_call_policies()
        else:
            return call_policies.return_internal_reference()

    def _create_impl(self):
        templates = declarations.templates
        call_invocation = declarations.call_invocation
        ns_name = code_repository.array_1.namespace
        if declarations.is_const( self.array_type ):
            fn_name = 'register_const_array_1'
        else:
            fn_name = 'register_array_1'

        fn_def_tmpl_args = [ declarations.array_item_type(self.array_type).decl_string
                             , str( declarations.array_size(self.array_type) ) ]
        if not self.call_policies.is_default():
            fn_def_tmpl_args.append( 
                self.call_policies.create(self, call_policies.CREATION_POLICY.AS_TEMPLATE_ARGUMENT ) )

        fn_def = templates.join( '::'.join( [ns_name, fn_name] ), fn_def_tmpl_args )
        return call_invocation.join( fn_def, [ '"%s"' % self._create_name() ] ) + ';'

    def _get_system_files_impl( self ):
        return [code_repository.array_1.file_name]
