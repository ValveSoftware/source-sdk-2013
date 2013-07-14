# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import types
from . import scoped
from . import calldef
from . import compound
from . import algorithm
from . import code_creator
from . import smart_pointers
from . import declaration_based
from . import registration_based
from pygccxml import declarations

class class_declaration_t( scoped.scoped_t
                           , registration_based.registration_based_t ):
    def __init__(self, class_inst ):
        scoped.scoped_t.__init__( self, declaration=class_inst )
        registration_based.registration_based_t.__init__( self )
        self.works_on_instance = False

    def _generate_class_definition(self):
        class_identifier = algorithm.create_identifier( self, '::boost::python::class_' )
        return declarations.templates.join( class_identifier, [self.decl_identifier] )

    def _generate_code_no_scope(self):
        result = []
        result.append( self._generate_class_definition() + '("%s")' % self.declaration.alias )
        for x in self.creators:
            code = x.create()
            tmpl = '%s.%s'
            if self.is_comment( code ):
                tmpl = '%s%s'
            if code:
                result.append( self.indent( tmpl % ( os.linesep, code ) ) )
        result.append( ';' )
        return ''.join( result )

    @property
    def class_var_name(self):
        return self.declaration.class_var_name

    def is_exposed_using_scope(self):
        if self.declaration.always_expose_using_scope:
            return True
        return bool( [cc for cc in self.creators if not cc.works_on_instance] )

    @property
    def typedef_name( self ):
        return self.class_var_name + '_t'

    def _generate_code_with_scope(self):
        result = []
        scope_var_name = self.alias + '_scope'
        result.append( 'typedef ' + self._generate_class_definition() + ' ' + self.typedef_name + ';')
        result.append( self.typedef_name + ' ' + self.class_var_name )
        result[-1] = result[-1] + ' = '+ self.typedef_name + '("%s");' % self.declaration.alias

        result.append( algorithm.create_identifier( self, '::boost::python::scope' ) )
        result[-1] = result[-1] + ' ' + scope_var_name
        result[-1] = result[-1] + '( %s );' % self.class_var_name

        for x in self.creators:
            if not x.works_on_instance:
                result.append( x.create() )
            else:
                result.append( '%s.%s;' % ( self.class_var_name, x.create() ) )

        code = os.linesep.join( result )

        result = [ '{ //scope begin' ]
        result.append( self.indent( code ) )
        result.append( '} //scope end' )

        return os.linesep.join( result )

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''
        if self.is_exposed_using_scope():
            return self._generate_code_with_scope()
        else:
            return self._generate_code_no_scope()

    def _get_system_files_impl( self ):
        return []

class class_t( scoped.scoped_t, registration_based.registration_based_t ):
    """
    Creates boost.python code that needed to export a class
    """
    def __init__(self, class_inst, wrapper=None ):
        scoped.scoped_t.__init__( self, declaration=class_inst )
        registration_based.registration_based_t.__init__( self )
        self._wrapper = wrapper
        self.works_on_instance = False

    def _get_wrapper( self ):
        return self._wrapper
    def _set_wrapper( self, new_wrapper ):
        self._wrapper = new_wrapper
    wrapper = property( _get_wrapper, _set_wrapper )

    def _get_held_type(self):
        return self.declaration.held_type
    def _set_held_type(self, held_type):
        assert isinstance( held_type, type(None) ) \
               or isinstance( held_type, smart_pointers.held_type_t ) \
               or isinstance( held_type, str )
        if isinstance( held_type, str ):
            assert held_type # should be non emptry string
        self.declaration.held_type = held_type
    held_type = property( _get_held_type, _set_held_type )

    def _exported_base_classes(self):
        if not self.declaration.bases:
            return {}, {}
        base_classes = {}
        for hierarchy_info in self.declaration.recursive_bases:
            if hierarchy_info.access_type == declarations.ACCESS_TYPES.PRIVATE:
                continue
            base_classes[ id( hierarchy_info.related_class ) ] = hierarchy_info
        base_classes_size = len( base_classes )
        creators = {}
        creators_len = 0
        for creator in algorithm.make_flatten_generator( self.top_parent.body.creators ):
            if isinstance( creator, class_t ) and id(creator.declaration) in base_classes:
                creators[ id(creator.declaration) ] = creator
                if len( creators ) == base_classes_size:
                    break #all classes has been found
        return base_classes, creators

    def _get_base_operators(self, base_classes, base_creators):
        #May be in future I will redefine operators on wrapper class
        #thus I will support [protected|private] [ [not|pure|] virtual] operators.
        operator_creators = []
        for base_creator in list(base_creators.values()):
            hierarchy_info = base_classes[ id( base_creator.declaration )]
            if hierarchy_info.access_type != declarations.ACCESS_TYPES.PUBLIC:
                continue
            base_operator_creators = [creator for creator in base_creator.creators if isinstance( creator, calldef.operator_t )
                                                and isinstance( creator.declaration, declarations.member_operator_t )
                                                and creator.declaration.access_type
                                                    == declarations.ACCESS_TYPES.PUBLIC]
            operator_creators.extend( base_operator_creators )
        return operator_creators

    def _generate_noncopyable(self):
        noncopyable_vars = self.declaration.find_noncopyable_vars()
        copy_constr = self.declaration.find_copy_constructor()

        if self.declaration.noncopyable \
           or copy_constr and copy_constr.is_artificial and noncopyable_vars:
            return algorithm.create_identifier( self, '::boost::noncopyable' )

    def _generate_bases(self, base_creators):
        bases = []
        assert isinstance( self.declaration, declarations.class_t )
        for base_desc in self.declaration.bases:
            assert isinstance( base_desc, declarations.hierarchy_info_t )
            if base_desc.access != declarations.ACCESS_TYPES.PUBLIC:
                continue
            if id(base_desc.related_class) in base_creators:
                bases.append( algorithm.create_identifier( self, base_desc.related_class.partial_decl_string ) )
            elif base_desc.related_class.already_exposed:
                bases.append( base_desc.related_class.partial_decl_string )
        if not bases:
            return None
        bases_identifier = algorithm.create_identifier( self, '::boost::python::bases' )
        return declarations.templates.join( bases_identifier, bases )

    def _generated_held_type(self):
        if isinstance( self.held_type, smart_pointers.held_type_t ):
            return self.held_type.create( self )
        elif isinstance( self.held_type, str):
            return self.held_type
        else:
            return None

    def _generate_class_definition(self, base_creators):
        class_identifier = algorithm.create_identifier( self, '::boost::python::class_' )
        args = []

        held_type = self._generated_held_type()
        if self.wrapper:
            if self.declaration.exposed_class_type == self.declaration.EXPOSED_CLASS_TYPE.WRAPPER:
                args.append( self.wrapper.full_name )
            else:
                if not self.target_configuration.boost_python_has_wrapper_held_type \
                   or self.declaration.require_self_reference:
                    args.append( self.decl_identifier )
                if self.declaration.require_self_reference:
                    if not held_type:
                        args.append( self.wrapper.full_name )
                else:
                    args.append( self.wrapper.full_name )
        else:
            args.append( self.decl_identifier )

        bases = self._generate_bases(base_creators)
        if bases:
            args.append( bases )

        if held_type:
            args.append( held_type )
        notcopyable = self._generate_noncopyable()
        if notcopyable:
            args.append( notcopyable )
        return declarations.templates.join( class_identifier, args)

    def _generate_constructor(self):
        result = []
        result.append( '(' )
        result.append( ' "%s"' % self.alias )
        if self.documentation:
            result.append( ', %s' % self.documentation )
        used_init = None
        inits = [x for x in self.creators if isinstance( x, calldef.constructor_t )]

        trivial_constructor = self.declaration.find_trivial_constructor()

        if self.declaration.no_init:
            result.append( ", " )
            result.append( algorithm.create_identifier( self, '::boost::python::no_init' ) )
        else:
            if inits:
                used_init = inits[0]
                result.append( ", " )
                result.append( used_init.create_init_code() )
        result.append( ' )' )
        return ( ''.join( result ), used_init )

    def _generate_code_no_scope(self):
        result = []
        base_classes, base_creators = self._exported_base_classes()
        result.append( self._generate_class_definition(base_creators) )
        class_constructor, used_init = self._generate_constructor()
        result.append( class_constructor )
        creators = self.creators
        if self.declaration.redefine_operators:
            creators = self.creators + self._get_base_operators(base_classes, base_creators)
        for x in creators:
            if not ( x is used_init ):
                code = x.create()
                tmpl = '%s.%s'
                if self.is_comment( code ):
                    tmpl = '%s%s'
                if code:
                    result.append( self.indent( tmpl % ( os.linesep, code ) ) )
        result.append( ';' )
        return ''.join( result )

    @property
    def class_var_name(self):
        return self.declaration.class_var_name

    @property
    def typedef_name( self ):
        return self.class_var_name + '_t'

    def create_typedef_code( self ):
        base_classes, base_creators = self._exported_base_classes()
        return 'typedef ' + self._generate_class_definition(base_creators) + ' ' + self.typedef_name + ';'


    def _generate_code_with_scope(self):
        result = []
        scope_var_name = self.alias + '_scope'
        base_classes, base_creators = self._exported_base_classes()
        result.append( 'typedef ' + self._generate_class_definition(base_creators) + ' ' + self.typedef_name + ';')
        result.append( self.typedef_name + ' ' + self.class_var_name )
        result[-1] = result[-1] + ' = '
        class_constructor, used_init = self._generate_constructor()
        result[-1] = result[-1] + self.typedef_name + class_constructor
        result[-1] = result[-1] + ';'

        result.append( algorithm.create_identifier( self, '::boost::python::scope' ) )
        result[-1] = result[-1] + ' ' + scope_var_name
        result[-1] = result[-1] + '( %s );' % self.class_var_name

        creators = self.creators
        if self.declaration.redefine_operators:
            creators = self.creators + self._get_base_operators(base_classes, base_creators)

        for x in creators:
            if x is used_init:
                continue
            if isinstance( x, ( calldef.calldef_t, calldef.calldef_overloads_t ) ):
                x.works_on_instance = False
                code = x.create()
                if code:
                    result.append( code )
                continue
            if not x.works_on_instance:
                code = x.create()
                if code:
                    result.append( code )
            else:
                result.append( '%s.%s;' % ( self.class_var_name, x.create() ) )

        code = os.linesep.join( result )

        result = [ '{ //%s' % declarations.full_name( self.declaration, with_defaults=False ) ]
        result.append( self.indent( code ) )
        result.append( '}' )

        return os.linesep.join( result )

    def is_exposed_using_scope(self):
        if self.declaration.always_expose_using_scope:
            return True
        return bool( [cc for cc in self.creators if not cc.works_on_instance] )

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''
        if self.is_exposed_using_scope():
            return self._generate_code_with_scope()
        else:
            return self._generate_code_no_scope()

    def _get_system_files_impl( self ):
        return []

class class_wrapper_t( scoped.scoped_t ):
    """
    creates C++ code, which creates wrapper around a class
    """

    def __init__(self, declaration, class_creator ):
        scoped.scoped_t.__init__( self, declaration=declaration )
        self._class_creator = class_creator
        self._base_wrappers = []

    def _get_wrapper_alias( self ):
        return self.declaration.wrapper_alias
    def _set_wrapper_alias( self, walias ):
        self.declaration.wrapper_alias = walias
    wrapper_alias = property( _get_wrapper_alias, _set_wrapper_alias )

    @property
    def base_wrappers( self ):
        if self.declaration.is_abstract and not self._base_wrappers:
            bases = [ hi.related_class for hi in self.declaration.bases ]
            creators_before_me = algorithm.creators_affect_on_me( self )
            self._base_wrappers \
                = [creator for creator in creators_before_me if isinstance( creator, class_wrapper_t )
                                          and creator.declaration in bases]
        return self._base_wrappers

    @property
    def exposed_identifier(self):
        return algorithm.create_identifier( self, self.declaration.partial_decl_string )

    @property
    def class_creator(self):
        return self._class_creator

    @property
    def full_name( self ):
        if not isinstance( self.parent, class_wrapper_t ):
            return self.declaration.wrapper_alias
        else:
            full_name = [self.wrapper_alias]
            #may be we deal with enum
            parent = self.parent
            while isinstance( parent, class_wrapper_t ):
                full_name.append( parent.wrapper_alias )
                parent = parent.parent
            full_name.reverse()
            return '::'.join( full_name )

    @property
    def held_type(self):
        return self._class_creator.held_type

    @property
    def boost_wrapper_identifier(self):
        boost_wrapper = algorithm.create_identifier( self, '::boost::python::wrapper' )
        return declarations.templates.join( boost_wrapper, [self.exposed_identifier] )

    def _create_bases(self):
        return ', '.join( [self.exposed_identifier, self.boost_wrapper_identifier] )

    def _create_impl(self):
        if self.declaration.already_exposed:
            return ''
        answer = ['struct %s : %s {' % ( self.wrapper_alias, self._create_bases() )]
        answer.append( '' )
        answer.append( self.create_internal_code( self.creators )  )
        answer.append( '' )
        answer.append( '};' )
        return os.linesep.join( answer )

    def _get_system_files_impl( self ):
        return []


ctypes_base_classes = {
    declarations.CLASS_TYPES.CLASS : 'Structure'
    , declarations.CLASS_TYPES.UNION : 'Union'
    , declarations.CLASS_TYPES.STRUCT : 'Structure'
}

class class_introduction_t(compound.compound_t, declaration_based.declaration_based_t):
    def __init__( self, class_ ):
        compound.compound_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, class_ )

    @property
    def ctypes_base_class( self ):
        global ctypes_base_classes
        return ctypes_base_classes[ self.declaration.class_type ]

    def _create_impl(self):
        result = []
        result.append( "class %(alias)s(ctypes.%(base)s):"
                       % dict( alias=self.alias, base=self.ctypes_base_class ) )
        result.append( self.indent( '"""class %s"""' % self.decl_identifier ) )
        if self.creators:
            result.append( self.indent( '' ) )
        result.append( compound.compound_t.create_internal_code( self.creators ) )

        if isinstance( self.declaration.parent, declarations.namespace_t ) \
           and self.declaration.parent is not self.declaration.top_parent: #not a global namespace
            result.append("")
            result.append( '%(ns_full_name)s = %(name)s'
                           % dict( ns_full_name=self.complete_py_name, name=self.alias ))
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []


class class_declaration_introduction_t(code_creator.code_creator_t, declaration_based.declaration_based_t):
    def __init__( self, class_declaration ):
        code_creator.code_creator_t.__init__(self)
        declaration_based.declaration_based_t.__init__( self, class_declaration )

    def _create_impl(self):
        result = []
        result.append( "class %s(ctypes.Structure):" % self.alias )
        result.append( self.indent( '"""class declaration %s"""' % self.decl_identifier ) )
        result.append( self.indent( '_fields_  = []' ) )

        if isinstance( self.declaration.parent, declarations.namespace_t ) \
           and self.declaration.parent is not self.declaration.top_parent: #not a global namespace
            result.append( '%(ns_full_name)s = %(name)s'
                           % dict( ns_full_name=self.complete_py_name, name=self.alias ))
        return os.linesep.join( result )

    def _get_system_files_impl( self ):
        return []
