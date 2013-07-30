# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import types
from . import algorithm
from . import code_creator
from . import declaration_based
from . import registration_based
from pygccxml import declarations

class indexing_suite1_t( registration_based.registration_based_t
                         , declaration_based.declaration_based_t ):
    def __init__(self, container ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=container )

    @property
    def configuration( self ):
        return self.declaration.indexing_suite

    @property
    def container( self ):
        return self.declaration

    def guess_suite_name( self ):
        if self.container.name.startswith( 'vector' ):
            return 'boost::python::vector_indexing_suite'
        else:
            return 'boost::python::map_indexing_suite'

    def _create_suite_declaration( self ):
        suite_identifier = algorithm.create_identifier( self, self.guess_suite_name() )
        args = [ self.container.partial_decl_string ]
        try:
            no_proxy = str( self.configuration.no_proxy ).lower()
        except:
            no_proxy = 'false'
        if self.configuration.derived_policies:
            args.append( no_proxy )
            args.append( self.configuration.derived_policies )
        else:
            if 'true' == no_proxy:
                args.append( no_proxy)
        return declarations.templates.join( suite_identifier, args )

    def _create_impl(self):
        return "def( %s() )" %  self._create_suite_declaration()

    def _get_system_files_impl( self ):
        return self.configuration.include_files

class indexing_suite2_t( registration_based.registration_based_t
                         , declaration_based.declaration_based_t ):
    def __init__(self, container ):
        registration_based.registration_based_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=container )
        self.__method_mask_var_name = "methods_mask"
        self.works_on_instance = not self.does_user_disable_methods()

    def does_user_disable_methods( self ):
        return bool( self.declaration.indexing_suite.disabled_methods_groups ) \
               or bool( self.declaration.indexing_suite.disable_methods )

    def generate_algorithm_mask( self ):
        disable = []
        for group in self.declaration.indexing_suite.disabled_methods_groups:
            group_id = algorithm.create_identifier(self, "::boost::python::indexing::%s_methods" % group )
            disable.append( group_id )
        for method in self.declaration.indexing_suite.disable_methods:
            method_id = algorithm.create_identifier(self, "::boost::python::indexing::method_" + method )
            disable.append( method_id )
        answer = [ 'unsigned long const %s = ' % self.__method_mask_var_name ]
        answer.append( algorithm.create_identifier(self, "::boost::python::indexing::all_methods" ) )
        answer.append( ' & ~' )
        if 1 == len ( disable ):
            answer.append( disable[0] )
        else:
            answer.append( '( ' )
            answer.append( ' |  '.join( disable ) )
            answer.append( ' ) ' )
        answer.append( ';' )
        return ''.join( answer )

    def _create_impl( self ):
        if self.declaration.already_exposed:
            return ''

        answer = []
        if self.does_user_disable_methods():
            answer.append( self.generate_algorithm_mask() )
            answer.append( os.linesep )
        if not self.works_on_instance:
            answer.append( '%s.def( ' % self.parent.class_var_name)
        else:
            answer.append( 'def( ' )
        bpi = algorithm.create_identifier(self, "::boost::python::indexing" )
        if self.declaration.indexing_suite.use_container_suite:
            answer.append( bpi + '::container_suite' )
        else:
            container_name = self.declaration.name.split( '<' )[0]
            if container_name.startswith( 'hash_' ):
                container_name = container_name[len( 'hash_'):]
            answer.append( bpi + '::' + container_name + '_suite' )
        answer.append( '< ' )
        answer.append( self.decl_identifier )
        if self.does_user_disable_methods():
            answer.append( self.PARAM_SEPARATOR )
            answer.append( self.__method_mask_var_name )
        answer.append( ' >' )
        if self.declaration.indexing_suite.call_policies \
           and not self.declaration.indexing_suite.call_policies.is_default():
            answer.append( '::with_policies(%s)'
                           % self.declaration.indexing_suite.call_policies.create( self )  )
        else:
            answer.append( '()' )
        answer.append( ' )' )
        if not self.works_on_instance:
            answer.append( ';' )
        return ''.join( answer )

    def _get_system_files_impl( self ):
        return self.declaration.indexing_suite.include_files

class value_traits_t( code_creator.code_creator_t
                      , declaration_based.declaration_based_t ):
    def __init__( self, value_class ):
        code_creator.code_creator_t.__init__( self )
        declaration_based.declaration_based_t.__init__( self, declaration=value_class )

    def generate_value_traits( self ):
        tmpl = os.linesep.join([
              "namespace boost { namespace python { namespace indexing {"
            , ""
            , "template<>"
            , "struct value_traits< %(value_class)s >{"
            , ""
            , self.indent( "static bool const equality_comparable = %(has_equal)s;" )
            , self.indent( "%(equal_to)s" )
            , ""
            , self.indent( "static bool const less_than_comparable = %(has_lessthan)s;" )
            , self.indent( "%(less)s" )
            , ""
            , self.indent( "template<typename PythonClass, typename Policy>" )
            , self.indent( "static void visit_container_class(PythonClass &, Policy const &){" )
            , self.indent( "%(visitor_helper_body)s", 2 )
            , self.indent( "}" )
            , ""
            , "};"
            , ""
            , "}/*indexing*/ } /*python*/ } /*boost*/"
        ])

        less = ''
        if self.declaration.less_than_comparable:
            less = "typedef std::less< %s > less;" % self.decl_identifier

        equal_to = ''
        if self.declaration.equality_comparable:
            equal_to = "typedef std::equal_to< %s > equal_to;" % self.decl_identifier

        return tmpl % { 'value_class' : self.decl_identifier
                        , 'has_equal' : str( bool( self.declaration.equality_comparable ) ) .lower()
                        , 'equal_to' : equal_to
                        , 'has_lessthan' : str( bool( self.declaration.less_than_comparable ) ).lower()
                        , 'less' : less
                        , 'visitor_helper_body' : '' }

    def generate_value_class_fwd_declaration( self ):
        pass # for inner class this code will generate error :-((((

    def _create_impl( self ):
        #if self.declaration.already_exposed:
        #    return ''
        #This is the error to skip generation in case the class is already exposed,
        #because we still expose container, so it needs to know how to work with
        #the value_type
        return self.generate_value_traits()

    def _get_system_files_impl( self ):
        return ['indexing_suite/value_traits.hpp']
