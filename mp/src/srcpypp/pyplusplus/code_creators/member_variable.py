# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from . import algorithm
from . import code_creator
from . import ctypes_formatter
from . import declaration_based
from . import registration_based
from pygccxml import declarations
from pyplusplus import code_repository
from pyplusplus.decl_wrappers import call_policies
from pyplusplus.decl_wrappers import python_traits

class member_variable_base_t( registration_based.registration_based_t
                              , declaration_based.declaration_based_t ):
    """
    Base class for all member variables code creators. Mainly exists to
    simplify file writers algorithms.
    """

    def __init__(self, variable, wrapper=None ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)
        self._wrapper = wrapper

    def _get_wrapper( self ):
        return self._wrapper
    def _set_wrapper( self, new_wrapper ):
        self._wrapper = new_wrapper
    wrapper = property( _get_wrapper, _set_wrapper )

    def _get_system_files_impl( self ):
        files = []
        if self.declaration.getter_call_policies:
            files.append( self.declaration.getter_call_policies.header_file )
        if self.declaration.setter_call_policies:
            files.append( self.declaration.setter_call_policies.header_file )
        return files

class member_variable_t( member_variable_base_t ):
    """
    Creates boost.python code that exposes member variable.
    """
    def __init__(self, variable, wrapper=None ):
        member_variable_base_t.__init__( self, variable=variable, wrapper=wrapper )

    #>    On Wednesday, 19. April 2006 23:05, Ralf W. Grosse-Kunstleve wrote:
    #>   .add_property("p", make_function(&A::get_p, return_value_policy<reference_existing_object>()))
    def _generate_for_pointer( self ):
        doc = '' #static property does not support documentation
        if self.declaration.type_qualifiers.has_static:
            add_property = 'add_static_property'
        else:
            if self.documentation:
                doc = self.documentation
            add_property = 'add_property'
        answer = [ add_property ]
        answer.append( '( ' )
        answer.append('"%s"' % self.alias)
        answer.append( self.PARAM_SEPARATOR )

        #according to David Abrahams:
        #http://mail.python.org/pipermail/c++-sig/2003-January/003276.html
        call_pol = call_policies.return_internal_reference().create( self )
        make_function = algorithm.create_identifier( self, '::boost::python::make_function' )

        answer.append( '%(mk_func)s( (%(getter_type)s)(&%(wfname)s), %(call_pol)s )'
                       % { 'mk_func' : make_function
                           , 'getter_type' : self.wrapper.getter_type
                           , 'wfname' : self.wrapper.getter_full_name
                           , 'call_pol' : call_pol } )

        #don't generate setter method, right now I don't know how to do it.
        if self.wrapper.has_setter:
            answer.append( self.PARAM_SEPARATOR )
            call_pol = ''
            if not self.declaration.type_qualifiers.has_static:
                call_pol = ", " + call_policies.with_custodian_and_ward_postcall( 1, 2 ).create(self)
            answer.append( '%(mk_func)s( (%(setter_type)s)(&%(wfname)s)%(call_pol)s )'
                       % { 'mk_func' : make_function
                           , 'setter_type' : self.wrapper.setter_type
                           , 'wfname' : self.wrapper.setter_full_name
                           , 'call_pol' : call_pol } )
        if doc:
            answer.append( self.PARAM_SEPARATOR )
            answer.append( doc )
        answer.append( ' ) ' )

        code = ''.join( answer )
        if len( code ) <= self.LINE_LENGTH:
            return code
        else:
            for i in range( len( answer ) ):
                if answer[i] == self.PARAM_SEPARATOR:
                    answer[i] = os.linesep + self.indent( self.indent( self.indent( answer[i] ) ) )
            return ''.join( answer )

    def _generate_for_none_pointer( self ):
        tmpl = None
        if self.declaration.type_qualifiers.has_static:
            tmpl = '%(access)s( "%(alias)s", %(name)s%(doc)s )'
        else:
            tmpl = '%(access)s( "%(alias)s", &%(name)s%(doc)s )'

        access = 'def_readwrite'
        if self.declaration.is_read_only:
            access = 'def_readonly'
        doc = ''
        if self.documentation:
            doc = ', %s' % self.documentation
        result = tmpl % {
                    'access' : access
                    , 'alias' : self.alias
                    , 'name' : self.decl_identifier
                    , 'doc' : doc }

        return result

    def _generate_using_functions( self ):
        doc = ''
        add_property = ''
        make_getter = algorithm.create_identifier( self, '::boost::python::make_getter')
        make_setter = algorithm.create_identifier( self, '::boost::python::make_setter')
        if self.declaration.type_qualifiers.has_static:
            add_property = 'add_static_property'
        else:
            if self.documentation:
                doc = self.documentation
            add_property = 'add_property'
        add_property_args = [ '"%s"' % self.alias ]
        getter_code = declarations.call_invocation.join(
                          make_getter
                        , [ '&' + self.decl_identifier
                            , self.declaration.getter_call_policies.create( self ) ]
                        , os.linesep + self.indent( self.PARAM_SEPARATOR, 6) )

        add_property_args.append( getter_code )
        if not self.declaration.is_read_only:
            setter_code = ''
            setter_args = [ '&' + self.decl_identifier ]
            if self.declaration.setter_call_policies \
               and not self.declaration.setter_call_policies.is_default():
                   setter_args.append( self.declaration.setter_call_policies.create( self ) )
            setter_code = declarations.call_invocation.join(
                              make_setter
                            , setter_args
                            , os.linesep + self.indent( self.PARAM_SEPARATOR, 6) )
            add_property_args.append( setter_code)
        if doc:
            add_property_args.append( doc )
        return declarations.call_invocation.join(
                    add_property
                    , add_property_args
                    , os.linesep + self.indent( self.PARAM_SEPARATOR, 4 ) )

    def _create_impl( self ):
        if declarations.is_pointer( self.declaration.type ):
            return self._generate_for_pointer()
        elif self.declaration.apply_smart_ptr_wa or self.declaration.use_make_functions:
            return self._generate_using_functions()
        else:
            return self._generate_for_none_pointer()

class member_variable_wrapper_t( code_creator.code_creator_t
                                 , declaration_based.declaration_based_t ):
    """
    creates get/set accessors for class variables, that has type pointer
    """
    #TODO: give user a way to set call policies
    #      treat void* pointer
    indent = code_creator.code_creator_t.indent
    MV_GET_TEMPLATE = os.linesep.join([
          'static %(type)s get_%(name)s(%(cls_type)s inst ){'
        , indent( 'return inst.%(name)s;' )
        , '}'
        , ''
    ])

    MV_STATIC_GET_TEMPLATE = os.linesep.join([
          'static %(type)s get_%(name)s(){'
        , indent( 'return %(cls_type)s::%(name)s;' )
        , '}'
        , ''
    ])

    MV_SET_TEMPLATE = os.linesep.join([
          'static void set_%(name)s( %(cls_type)s inst, %(type)s new_value ){ '
        , indent( 'inst.%(name)s = new_value;' )
        , '}'
        , ''
    ])

    MV_STATIC_SET_TEMPLATE = os.linesep.join([
          'static void set_%(name)s( %(type)s new_value ){ '
        , indent( '%(cls_type)s::%(name)s = new_value;' )
        , '}'
        , ''
    ])

    def __init__(self, variable ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)

    def _get_getter_full_name(self):
        return self.parent.full_name + '::' + 'get_' + self.declaration.name
    getter_full_name = property( _get_getter_full_name )

    def inst_arg_type( self, has_const ):
        inst_arg_type = declarations.declarated_t( self.declaration.parent )
        if has_const:
            inst_arg_type = declarations.const_t(inst_arg_type)
        inst_arg_type = declarations.reference_t(inst_arg_type)
        return inst_arg_type

    def _get_getter_type(self):
        if self.declaration.type_qualifiers.has_static:
            arguments_types=[]
        else:
            arguments_types=[ self.inst_arg_type(True) ]

        return declarations.free_function_type_t.create_decl_string(
                return_type=self.declaration.type
                , arguments_types=arguments_types
                , with_defaults=False)
    getter_type = property( _get_getter_type )

    def _get_setter_full_name(self):
        return self.parent.full_name + '::' + 'set_' + self.declaration.name
    setter_full_name = property(_get_setter_full_name)

    def _get_setter_type(self):
        if self.declaration.type_qualifiers.has_static:
            arguments_types=[ self.declaration.type ]
        else:
            arguments_types=[ self.inst_arg_type(False), self.declaration.type  ]

        return declarations.free_function_type_t.create_decl_string(
                return_type=declarations.void_t()
                , arguments_types=arguments_types
                , with_defaults=False)
    setter_type = property( _get_setter_type )

    def _get_has_setter( self ):
        return not declarations.is_const( self.declaration.type )
    has_setter = property( _get_has_setter )

    def _create_impl(self):
        answer = []
        if self.declaration.type_qualifiers.has_static:
            substitutions = {
                'type' : self.declaration.type.decl_string
                , 'name' : self.declaration.name
                , 'cls_type' : declarations.full_name( self.declaration.parent )
            }
            answer.append( self.MV_STATIC_GET_TEMPLATE % substitutions)
            if self.has_setter:
                answer.append( self.MV_STATIC_SET_TEMPLATE % substitutions )
        else:
            answer.append( self.MV_GET_TEMPLATE % {
                'type' : self.declaration.type.decl_string
                , 'name' : self.declaration.name
                , 'cls_type' : self.inst_arg_type( has_const=True ) })
            if self.has_setter:
                answer.append( self.MV_SET_TEMPLATE % {
                'type' : self.declaration.type.decl_string
                , 'name' : self.declaration.name
                , 'cls_type' : self.inst_arg_type( has_const=False ) })
        return os.linesep.join( answer )

    def _get_system_files_impl( self ):
        return []

class bit_field_t( member_variable_base_t ):
    """
    Creates boost.python code that exposes bit fields member variables
    """
    def __init__(self, variable, wrapper ):
        member_variable_base_t.__init__( self, variable=variable, wrapper=wrapper )

    def _create_impl( self ):
        answer = [ 'add_property' ]
        answer.append( '( ' )
        answer.append('"%s"' % self.alias)
        answer.append( self.PARAM_SEPARATOR )

        make_function = algorithm.create_identifier( self, '::boost::python::make_function' )

        answer.append( '%(mk_func)s( (%(getter_type)s)(&%(wfname)s) )'
                       % { 'mk_func' : make_function
                           , 'getter_type' : self.wrapper.getter_type
                           , 'wfname' : self.wrapper.getter_full_name } )

        if self.wrapper.has_setter:
            answer.append( self.PARAM_SEPARATOR )
            answer.append( '%(mk_func)s( (%(setter_type)s)(&%(wfname)s) )'
                       % { 'mk_func' : make_function
                           , 'setter_type' : self.wrapper.setter_type
                           , 'wfname' : self.wrapper.setter_full_name } )
                           
        if self.documentation:
            answer.append( self.PARAM_SEPARATOR )
            answer.append( self.documentation )
        answer.append( ' ) ' )

        code = ''.join( answer )
        if len( code ) <= self.LINE_LENGTH:
            return code
        else:
            for i in range( len( answer ) ):
                if answer[i] == self.PARAM_SEPARATOR:
                    answer[i] = os.linesep + self.indent( self.indent( self.indent( answer[i] ) ) )
            return ''.join( answer )

class bit_field_wrapper_t( code_creator.code_creator_t
                           , declaration_based.declaration_based_t ):
    """
    creates get/set accessors for bit fields
    """
    indent = code_creator.code_creator_t.indent
    GET_TEMPLATE =os.linesep.join([
          'static %(type)s get_%(name)s(%(cls_type)s inst ){'
        , indent( 'return inst.%(name)s;' )
        , '}'
        , ''
    ])

    SET_TEMPLATE = os.linesep.join([
          'static void set_%(name)s( %(cls_type)s inst, %(type)s new_value ){ '
        , indent( 'inst.%(name)s = new_value;' )
        , '}'
        , ''
    ])

    def __init__(self, variable ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)

    def _get_getter_full_name(self):
        return self.parent.full_name + '::' + 'get_' + self.declaration.name
    getter_full_name = property( _get_getter_full_name )

    def inst_arg_type( self, has_const ):
        inst_arg_type = declarations.declarated_t( self.declaration.parent )
        if has_const:
            inst_arg_type = declarations.const_t(inst_arg_type)
        inst_arg_type = declarations.reference_t(inst_arg_type)
        return inst_arg_type

    def _get_getter_type(self):
        return declarations.free_function_type_t.create_decl_string(
                return_type=self.declaration.type
                , arguments_types=[ self.inst_arg_type(True) ]
                , with_defaults=False)
    getter_type = property( _get_getter_type )

    def _get_setter_full_name(self):
        return self.parent.full_name + '::' + 'set_' + self.declaration.name
    setter_full_name = property(_get_setter_full_name)

    def _get_setter_type(self):
        return declarations.free_function_type_t.create_decl_string(
                return_type=declarations.void_t()
                , arguments_types=[ self.inst_arg_type(False), self.declaration.type  ]
                , with_defaults=False)
    setter_type = property( _get_setter_type )

    def _get_has_setter( self ):
        return not declarations.is_const( self.declaration.type )
    has_setter = property( _get_has_setter )

    def _create_impl(self):
        answer = []
        answer.append( self.GET_TEMPLATE % {
            'type' : self.declaration.type.decl_string
            , 'name' : self.declaration.name
            , 'cls_type' : self.inst_arg_type( has_const=True ) })
        if self.has_setter:
            answer.append( self.SET_TEMPLATE % {
            'type' : self.declaration.type.decl_string
            , 'name' : self.declaration.name
            , 'cls_type' : self.inst_arg_type( has_const=False ) })
        return os.linesep.join( answer )

    def _get_system_files_impl( self ):
        return []

class array_mv_t( member_variable_base_t ):
    """
    Creates boost.python code that exposes array member variable.
    """
    def __init__(self, variable, wrapper ):
        member_variable_base_t.__init__( self, variable=variable, wrapper=wrapper )
        self.works_on_instance = False

    def _create_body( self ):
        answer = []
        answer.append( 'typedef %s;' % self.wrapper.wrapper_creator_type.create_typedef( 'array_wrapper_creator' ) )
        answer.append( os.linesep * 2 )

        doc = ''
        if self.declaration.type_qualifiers.has_static:
            answer.append( self.parent.class_var_name + '.add_static_property' )
        else:
            if self.documentation:
                doc = self.documentation
            answer.append( self.parent.class_var_name + '.add_property' )
        answer.append( '( ' )
        answer.append('"%s"' % self.declaration.name )
        answer.append( os.linesep + self.indent( self.PARAM_SEPARATOR ) )
        temp = [ algorithm.create_identifier( self, "::boost::python::make_function" ) ]
        temp.append( '( ' )
        temp.append( 'array_wrapper_creator(&%s)' % self.wrapper.wrapper_creator_full_name )
        if not self.declaration.type_qualifiers.has_static:
            temp.append( os.linesep + self.indent( self.PARAM_SEPARATOR, 6 ) )
            temp.append( call_policies.with_custodian_and_ward_postcall( 0, 1 ).create(self) )
        temp.append( ' )' )
        answer.append( ''.join( temp ) )
        if doc:
            answer.append( os.linesep )
            answer.append( self.PARAM_SEPARATOR )
            answer.append( doc )
        answer.append( ' );' )
        return ''.join( answer )

    def _create_impl( self ):
        answer = []
        answer.append( '{ //%s, type=%s' % ( self.declaration, self.declaration.type ) )
        answer.append( os.linesep * 2 )
        answer.append( self.indent( self._create_body() ) )
        answer.append( os.linesep )
        answer.append( '}' )
        return ''.join( answer )

    def _get_system_files_impl( self ):
        return []


class array_mv_wrapper_t( code_creator.code_creator_t
                          , declaration_based.declaration_based_t ):
    """registers array class"""
    def __init__(self, variable ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)

    @property
    def wrapper_type( self ):
        tmpl = "%(namespace)s::%(constness)sarray_1_t< %(item_type)s, %(array_size)d>"

        constness = ''
        if declarations.is_const( self.declaration.type ):
            constness = 'const_'
        result = tmpl % {
                'namespace' : code_repository.array_1.namespace
              , 'constness' : constness
              , 'item_type' : declarations.array_item_type( self.declaration.type ).decl_string
              , 'array_size': declarations.array_size( self.declaration.type )
        }
        return declarations.dummy_type_t( result )

    @property
    def wrapped_class_type( self ):
        wrapped_cls_type = declarations.declarated_t( self.declaration.parent )
        if declarations.is_const( self.declaration.type ):
            wrapped_cls_type = declarations.const_t( wrapped_cls_type )
        return declarations.reference_t( wrapped_cls_type )

    @property
    def wrapper_creator_type(self):
        if self.declaration.type_qualifiers.has_static:
            return declarations.free_function_type_t( return_type=self.wrapper_type )
        else:
            return declarations.free_function_type_t(
                    return_type=self.wrapper_type
                    , arguments_types=[self.wrapped_class_type] )

    @property
    def wrapper_creator_name(self):
        return '_'.join( ['pyplusplus', self.declaration.name, 'wrapper'] )

    @property
    def wrapper_creator_full_name(self):
        return '::'.join( [self.parent.full_name, self.wrapper_creator_name] )

    def _create_impl( self ):        
        tmpl = [ "static %(wrapper_type)s" ]
        if self.declaration.type_qualifiers.has_static:
            tmpl.append( "%(wrapper_creator_name)s(){" )
            tmpl.append( self.indent( "return %(wrapper_type)s( %(parent_class_type)s::%(mem_var_ref)s );" ) )
        else:
            tmpl.append( "%(wrapper_creator_name)s( %(wrapped_class_type)s inst ){" )
            tmpl.append( self.indent( "return %(wrapper_type)s( inst.%(mem_var_ref)s );" ) )
        tmpl.append( "}" )
        
        tmpl = os.linesep.join( tmpl )
        
        return tmpl % {
                'wrapper_type' : self.wrapper_type.decl_string
              , 'parent_class_type' : self.parent.declaration.partial_decl_string
              , 'wrapper_creator_name' : self.wrapper_creator_name
              , 'wrapped_class_type' : self.wrapped_class_type.decl_string
              , 'mem_var_ref' : self.declaration.name
            }

    def _get_system_files_impl( self ):
        return [code_repository.array_1.file_name]


class mem_var_ref_t( member_variable_base_t ):
    """
    creates get/set accessors for class member variable, that has type reference.
    """
    def __init__(self, variable, wrapper ):
        member_variable_base_t.__init__( self, variable=variable, wrapper=wrapper )
        self.param_sep = os.linesep + self.indent( self.PARAM_SEPARATOR, 2 )
        self.works_on_instance = False

    def _create_getter( self ):
        answer = ['def']
        answer.append( '( ' )
        answer.append( '"get_%s"' % self.alias)
        answer.append( self.param_sep )
        answer.append( '(%s)(&%s)'
                       % ( self.wrapper.getter_type.decl_string, self.wrapper.getter_full_name ) )
        if self.declaration.getter_call_policies:
            if not self.declaration.getter_call_policies.is_default():
                answer.append( self.param_sep )
                answer.append( self.declaration.getter_call_policies.create( self ) )
        else:
            answer.append( os.linesep + self.indent( '/* undefined call policies */', 2 ) )
        if self.documentation:
            answer.append( self.param_sep )
            answer.append( self.documentation )
        answer.append( ' )' )
        return ''.join( answer )

    def _create_setter( self ):
        answer = ['def']
        answer.append( '( ' )
        answer.append( '"set_%s"' % self.alias)
        answer.append( self.param_sep )
        answer.append( '(%s)(&%s)'
                       % ( self.wrapper.setter_type.decl_string, self.wrapper.setter_full_name ) )
        if self.declaration.setter_call_policies:
            if not self.declaration.setter_call_policies.is_default():
                answer.append( self.param_sep )
                answer.append( self.declaration.setter_call_policies.create( self ) )
        else:
            answer.append( os.linesep + self.indent( '/* undefined call policies */', 2 ) )
        answer.append( ' )' )
        return ''.join( answer )

    def _create_impl( self ):
        #TODO: fix me, should not force class scope usage
        answer = []
        class_var_name = self.parent.class_var_name
        answer.append( "%s.%s;" % (class_var_name, self._create_getter() ) )
        if self.wrapper.has_setter:
            answer.append( os.linesep )
            answer.append( "%s.%s;" % (class_var_name, self._create_setter() ) )
        return ''.join( answer )

class mem_var_ref_wrapper_t( code_creator.code_creator_t
                             , declaration_based.declaration_based_t ):
    """
    creates get/set accessors for class member variable, that has type reference.
    """

    indent = code_creator.code_creator_t.indent
    GET_TEMPLATE = os.linesep.join([
          'static %(type)s get_%(name)s( %(class_type)s& inst ) {'
        , indent( 'return inst.%(name)s;' )
        , '}'
        , ''
    ])

    SET_TEMPLATE = os.linesep.join([
          'static void set_%(name)s( %(class_type)s& inst, %(type)s new_value ){ '
        , indent( 'inst.%(name)s = new_value;' )
        , '}'
        , ''
    ])

    def __init__(self, variable ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=variable)

    def _get_getter_full_name(self):
        return self.parent.full_name + '::' + 'get_' + self.declaration.name
    getter_full_name = property( _get_getter_full_name )

    def _get_class_inst_type( self ):
        return declarations.declarated_t( self.declaration.parent )

    def _get_exported_var_type( self ):
        type_ = declarations.remove_reference( self.declaration.type )
        type_ = declarations.remove_const( type_ )
        if python_traits.is_immutable( type_ ):
            return type_
        else:
            return self.declaration.type

    def _get_getter_type(self):
        return declarations.free_function_type_t(
                return_type=self._get_exported_var_type()
                , arguments_types=[ declarations.reference_t( self._get_class_inst_type() ) ] )
    getter_type = property( _get_getter_type )

    def _get_setter_full_name(self):
        return self.parent.full_name + '::' + 'set_' + self.declaration.name
    setter_full_name = property(_get_setter_full_name)

    def _get_setter_type(self):
        return declarations.free_function_type_t(
                return_type=declarations.void_t()
                , arguments_types=[ declarations.reference_t( self._get_class_inst_type() )
                                    , self._get_exported_var_type() ] )
    setter_type = property( _get_setter_type )

    def _get_has_setter( self ):
        if declarations.is_const( declarations.remove_reference( self.declaration.type ) ):
            return False
        elif python_traits.is_immutable( self._get_exported_var_type() ):
            return True
        else:
            pass

        no_ref = declarations.remove_reference( self.declaration.type )
        no_const = declarations.remove_const( no_ref )
        base_type = declarations.remove_alias( no_const )
        if not isinstance( base_type, declarations.declarated_t ):
            return True #TODO ????
        decl = base_type.declaration
        if decl.is_abstract:
            return False
        if declarations.has_destructor( decl ) and not declarations.has_public_destructor( decl ):
            return False
        if not declarations.has_copy_constructor(decl):
            return False
        return True
    has_setter = property( _get_has_setter )

    def _create_impl(self):
        answer = []
        cls_type = algorithm.create_identifier( self, self.declaration.parent.decl_string )

        substitutions = dict( type=self._get_exported_var_type().decl_string
                              , class_type=cls_type
                              , name=self.declaration.name )
        answer.append( self.GET_TEMPLATE % substitutions )
        if self.has_setter:
            answer.append( self.SET_TEMPLATE % substitutions )
        return os.linesep.join( answer )

    def _get_system_files_impl( self ):
        return []

class member_variable_addressof_t( member_variable_base_t ):
    """
    Creates boost.python code that exposes address of member variable.

    This functionality is pretty powerful if you use it with "ctypes" -
    standard package.

    """
    def __init__(self, variable, wrapper=None ):
        member_variable_base_t.__init__( self, variable=variable, wrapper=wrapper )

    def has_setter( self ) :
        return declarations.is_pointer( self.declaration.type ) \
               and not declarations.is_const( self.declaration.type )

    def _create_m_var( self ):
        param_sep = self.PARAM_SEPARATOR
        if self.has_setter() or self.documentation:
            param_sep = os.linesep + self.indent( self.PARAM_SEPARATOR, 3 )
        answer = [ 'add_property' ]
        answer.append( '( ' )
        answer.append('"%s"' % self.alias)
        answer.append( param_sep )
        answer.append( 'pyplus_conv::make_addressof_getter(&%s)'
                       % self.decl_identifier )
        if self.has_setter():
            answer.append( param_sep )
            answer.append( 'pyplus_conv::make_address_setter(&%s)'
                           % self.decl_identifier )
        if self.documentation:
            answer.append( param_sep  )
            answer.append( self.documentation )
        answer.append( ' ) ' )
        return ''.join( answer )

    def _create_s_var( self ):
        return 'def( %(def_visitor)s("%(name)s", %(var)s) )' \
               % {   'def_visitor' : 'pyplus_conv::register_addressof_static_var'
                   , 'name' : self.alias
                   , 'var' : self.decl_identifier }


    def _create_impl( self ):
        if self.declaration.type_qualifiers.has_static:
            return self._create_s_var()
        else:
            return self._create_m_var()
    def _get_system_files_impl( self ):
        return [code_repository.ctypes_integration.file_name]


#TODO: don't hide public member variables
#TODO: how _fields_ should be defined in a class hierarchy
#TODO: fix 64bit issue with calculating vtable pointer size

class fields_definition_t(code_creator.code_creator_t, declaration_based.declaration_based_t):
    def __init__( self, class_ ):
        code_creator.code_creator_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, class_ )

    def has_unnamed_type( self, var ):
        type_ = declarations.remove_pointer( var.type )
        #~ type_ = declarations.remove_declarated( type_ )
        if declarations.class_traits.is_my_case( type_ ):
            cls = declarations.class_traits.get_declaration( type_ )
            return bool( not cls.name )
        else:
            return False

    def _create_impl(self):
        result = []
        anonymous_vars = self.declaration.vars( self.has_unnamed_type, recursive=False, allow_empty=True )
        if anonymous_vars:
            formated_vars = []
            for var in anonymous_vars:
                formated_vars.append( '"%s"' % var.alias )
            result.append( '%(complete_py_name)s._anonymous_ = [%(vars)s]'
                           % dict( complete_py_name=self.complete_py_name
                                   , vars=', '.join( formated_vars ) ) )

        result.append( '%(complete_py_name)s._fields_ = [ #class %(decl_identifier)s'
                       % dict( complete_py_name=self.complete_py_name
                               , decl_identifier=self.decl_identifier) )
        if self.declaration.has_vtable:
            result.append( self.indent( '("_vtable_", ctypes.POINTER(ctypes.c_void_p)),' ) )

        vars = self.declaration.vars( allow_empty=True, recursive=False )
        if not vars:
            result.append( self.indent( '("__empty__", ctypes.c_char * 4)' ) )
        else:
            vars = vars.to_list()
            vars.sort( key=lambda d: d.location.line )
            for v in vars:
                tmp = None
                type_as_str = ctypes_formatter.as_ctype( v.type, self.top_parent.treat_char_ptr_as_binary_data )
                if v.bits != None:
                    tmp = '("%(name)s", %(type)s, %(bits)d),' \
                          % dict( name=v.alias, type=type_as_str, bits=v.bits )
                else:
                    tmp = '("%(name)s", %(type)s),' % dict( name=v.alias, type=type_as_str )
                result.append( self.indent( tmp ) )
        result.append( ']' )
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []
