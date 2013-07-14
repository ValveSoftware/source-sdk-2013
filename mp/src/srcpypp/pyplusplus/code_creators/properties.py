# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import registration_based
from pygccxml import declarations

class property_t( registration_based.registration_based_t ):
    def __init__(self, property_def, wrapper=None ):
        registration_based.registration_based_t.__init__( self )
        self._property_def = property_def
        self.works_on_instance = False
        self._make_function = None

    @property
    def property_def( self ):
        return self._property_def

    @property
    def make_function_identifier( self ):
        if not self._make_function:
            self._make_function = algorithm.create_identifier( self, '::boost::python::make_function' )
        return self._make_function

    def create_function_type_alias_code( self, f, ftype_alias, exported_class_alias=None ):
        return 'typedef ' + f.function_type().create_typedef( ftype_alias, exported_class_alias ) + ';'

    def create_accessor_code( self, f, ftype_alias ):
        f_ref_code = '%s( &%s )' % ( ftype_alias, declarations.full_name( f ) )
        if f.call_policies and f.call_policies.is_default():
            return f_ref_code
        result = [ self.make_function_identifier ]
        result.append( '( ' )
        result.append( os.linesep )
        result.append( self.indent( '  ', 2 ) )
        result.append( f_ref_code )
        if f.call_policies:
            result.append( os.linesep )
            result.append( self.indent( ', ', 2 ) )
            result.append( f.call_policies.create( self ) )
        else:
            result.append( os.linesep )
            result.append( self.indent( '  ', 2 ) )
            result.append( '/* undefined call policies */' )
        result.append( ' ) ' )
        return ''.join( result )

    def has_long_line( self ):
        pd = self.property_def
        if pd.fget.call_policies and not pd.fget.call_policies.is_default():
            return True
        elif pd.fset or ( pd.fset and ( pd.fset.call_policies or not pd.fset.call_policies.is_default() ) ):
            return True
        elif pd.doc:
            return True
        else:
            return True

    def is_same_parent( self ):
        pd = self.property_def
        if not pd.fset:
            return False
        return pd.fget.parent is pd.fset.parent

    def create_class_typedef_on_demand( self, f, prefix='' ):
        if None is f:
            return ( None, None )
        if not isinstance( f.parent, declarations.class_t ):
            return ( None, None )
        if not declarations.templates.is_instantiation( f.parent.decl_string ):
            return ( None, None )
        cls_name = None
        cls_identifier = algorithm.create_identifier( self, f.parent.decl_string )
        if prefix:
            cls_name = prefix + 'class_t'
        else:
            cls_name = 'exported_class_t'
        return ( 'typedef %s %s;' % ( cls_identifier, cls_name ), cls_name )

    def create_property_code( self ):
        result = []
        param_sep = ', '
        if self.has_long_line():
            param_sep = os.linesep + self.indent( param_sep )

        fget_class_typedef_code, fget_class_alias = None, None
        fset_class_typedef_code, fset_class_alias = None, None
        if self.is_same_parent():
            fget_class_typedef_code, fget_class_alias \
                = self.create_class_typedef_on_demand( self.property_def.fget )
            fset_class_alias = fget_class_alias
            fset_class_typedef_code = fget_class_typedef_code
        else:
            fget_class_typedef_code, fget_class_alias \
                = self.create_class_typedef_on_demand( self.property_def.fget, 'fget_' )
            fset_class_typedef_code, fset_class_alias \
                = self.create_class_typedef_on_demand( self.property_def.fset, 'fset_' )

        if fget_class_typedef_code:
            result.append( fget_class_typedef_code )

        if fset_class_typedef_code and fset_class_typedef_code != fget_class_typedef_code:
            result.append( os.linesep )
            result.append( fset_class_typedef_code )

        if result:
            result.append( 2 * os.linesep )

        result.append( self.create_function_type_alias_code( self.property_def.fget, 'fget', fget_class_alias ) )
        if self.property_def.fset:
            result.append( os.linesep )
            result.append( self.create_function_type_alias_code( self.property_def.fset, 'fset', fset_class_alias ) )

        result.append( 2 * os.linesep )

        add_property = None
        if self.property_def.is_static:
            add_property = 'add_static_property'
        else:
            add_property = 'add_property'

        class_var_name = self.parent.class_var_name
        if self.has_long_line():
            result.append( '%s.%s( ' % ( class_var_name, add_property ) )
            result.append( os.linesep + self.indent( '"%s"' % self.property_def.name ) )
        else:
            result.append( '%s.%s( "%s"' % ( class_var_name, add_property, self.property_def.name ) )
        result.append( param_sep + self.create_accessor_code( self.property_def.fget, 'fget' ) )
        if self.property_def.fset:
            result.append( param_sep + self.create_accessor_code( self.property_def.fset, 'fset' ))
        if self.property_def.doc:
            result.append( param_sep + self.property_def.doc)
        result.append( ' );')
        return ''.join( result )

    def _create_impl( self ):
        result = []
        result.append( '{ //%s' % self.property_def )
        result.append( '' )
        result.append( self.indent( self.create_property_code() ) )
        result.append( '' )
        result.append( '}' )
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []





















