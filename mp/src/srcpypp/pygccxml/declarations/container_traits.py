# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines few algorithms, that deals with different properties of std containers
"""

import types
import string
from . import calldef
from . import cpptypes
from . import namespace
from . import templates
from . import type_traits
from . import class_declaration

std_namespaces = ( 'std', 'stdext', '__gnu_cxx' )

class defaults_eraser:
    @staticmethod
    def normalize( type_str ):
        return type_str.replace( ' ', '' )

    @staticmethod
    def replace_basic_string( cls_name ):
        strings = {
              'std::string' : ( 'std::basic_string<char,std::char_traits<char>,std::allocator<char> >'
                                , 'std::basic_string<char, std::char_traits<char>, std::allocator<char> >' )
            , 'std::wstring' : ( 'std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >'
                                 , 'std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >' ) }

        new_name = cls_name
        for short_name, long_names in strings.items():
            for lname in long_names:
                new_name = new_name.replace( lname, short_name )
        return new_name

    class recursive_impl:
        @staticmethod
        def decorated_call_prefix( cls_name, text, doit ):
            has_text = cls_name.startswith( text )
            if has_text:
                cls_name = cls_name[ len( text ): ]
            answer = doit( cls_name )
            if has_text:
                answer = text + answer
            return answer

        @staticmethod
        def decorated_call_suffix( cls_name, text, doit ):
            has_text = cls_name.endswith( text )
            if has_text:
                cls_name = cls_name[: len( text )]
            answer = doit( cls_name )
            if has_text:
                answer = answer + text
            return answer

        @staticmethod
        def erase_call( cls_name ):
            global find_container_traits
            c_traits = find_container_traits( cls_name )
            if not c_traits:
                return cls_name
            return c_traits.remove_defaults( cls_name )

        @staticmethod
        def erase_recursive( cls_name ):
            ri = defaults_eraser.recursive_impl
            no_std = lambda cls_name: ri.decorated_call_prefix( cls_name, 'std::', ri.erase_call )
            no_stdext = lambda cls_name: ri.decorated_call_prefix( cls_name, 'stdext::', no_std )
            no_gnustd = lambda cls_name: ri.decorated_call_prefix( cls_name, '__gnu_cxx::', no_stdext )
            no_const = lambda cls_name: ri.decorated_call_prefix( cls_name, 'const ', no_gnustd )
            no_end_const = lambda cls_name: ri.decorated_call_suffix( cls_name, ' const', no_const )
            return no_end_const( cls_name )

    @staticmethod
    def erase_recursive( cls_name ):
        return defaults_eraser.recursive_impl.erase_recursive( cls_name )

    @staticmethod
    def erase_allocator( cls_name, default_allocator='std::allocator' ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )
        if 2 != len( c_args ):
            return
        value_type = c_args[0]
        tmpl = string.Template( "$container< $value_type, $allocator<$value_type> >" )
        tmpl = tmpl.substitute( container=c_name, value_type=value_type, allocator=default_allocator )
        if defaults_eraser.normalize( cls_name ) == defaults_eraser.normalize( tmpl ):
            return templates.join( c_name, [defaults_eraser.erase_recursive( value_type )] )

    @staticmethod
    def erase_container( cls_name, default_container_name='std::deque' ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )
        if 2 != len( c_args ):
            return
        value_type = c_args[0]
        dc_no_defaults = defaults_eraser.erase_recursive( c_args[1] )
        if defaults_eraser.normalize( dc_no_defaults ) \
           != defaults_eraser.normalize( templates.join( default_container_name, [value_type] ) ):
            return
        return templates.join( c_name, [defaults_eraser.erase_recursive( value_type )] )

    @staticmethod
    def erase_container_compare( cls_name, default_container_name='std::vector', default_compare='std::less' ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )
        if 3 != len( c_args ):
            return
        dc_no_defaults = defaults_eraser.erase_recursive( c_args[1] )
        if defaults_eraser.normalize( dc_no_defaults ) \
           != defaults_eraser.normalize( templates.join( default_container_name, [c_args[0]] ) ):
            return
        dcomp_no_defaults = defaults_eraser.erase_recursive( c_args[2] )
        if defaults_eraser.normalize( dcomp_no_defaults ) \
           != defaults_eraser.normalize( templates.join( default_compare, [c_args[0]] ) ):
            return
        value_type = defaults_eraser.erase_recursive( c_args[0] )
        return templates.join( c_name, [value_type] )

    @staticmethod
    def erase_compare_allocator( cls_name, default_compare='std::less', default_allocator='std::allocator' ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )
        if 3 != len( c_args ):
            return
        value_type = c_args[0]
        tmpl = string.Template( "$container< $value_type, $compare<$value_type>, $allocator<$value_type> >" )
        tmpl = tmpl.substitute( container=c_name
                                , value_type=value_type
                                , compare=default_compare
                                , allocator=default_allocator )
        if defaults_eraser.normalize( cls_name ) == defaults_eraser.normalize( tmpl ):
            return templates.join( c_name, [defaults_eraser.erase_recursive( value_type )] )

    @staticmethod
    def erase_map_compare_allocator( cls_name, default_compare='std::less', default_allocator='std::allocator' ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )
        if 4 != len( c_args ):
            return
        key_type = c_args[0]
        mapped_type = c_args[1]
        tmpls = [
            string.Template( "$container< $key_type, $mapped_type, $compare<$key_type>, $allocator< std::pair< const $key_type, $mapped_type> > >" )
            , string.Template( "$container< $key_type, $mapped_type, $compare<$key_type>, $allocator< std::pair< $key_type const, $mapped_type> > >" )
            , string.Template( "$container< $key_type, $mapped_type, $compare<$key_type>, $allocator< std::pair< $key_type, $mapped_type> > >" )]
        for tmpl in tmpls:
            tmpl = tmpl.substitute( container=c_name
                                    , key_type=key_type
                                    , mapped_type=mapped_type
                                    , compare=default_compare
                                    , allocator=default_allocator )
            if defaults_eraser.normalize( cls_name ) == defaults_eraser.normalize( tmpl ):
                return templates.join( c_name
                                       , [ defaults_eraser.erase_recursive( key_type )
                                           , defaults_eraser.erase_recursive( mapped_type )] )


    @staticmethod
    def erase_hash_allocator( cls_name ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )
        if len( c_args ) < 3:
            return

        default_hash=None
        default_less='std::less'
        default_equal_to='std::equal_to'
        default_allocator='std::allocator'

        tmpl = None
        if 3 == len( c_args ):
            default_hash='hash_compare'
            tmpl = "$container< $value_type, $hash<$value_type, $less<$value_type> >, $allocator<$value_type> >"
        elif 4 == len( c_args ):
            default_hash='hash'
            tmpl = "$container< $value_type, $hash<$value_type >, $equal_to<$value_type >, $allocator<$value_type> >"
        else:
            return

        value_type = c_args[0]
        tmpl = string.Template( tmpl )
        for ns in std_namespaces:
            inst = tmpl.substitute( container=c_name
                                    , value_type=value_type
                                    , hash= ns + '::' + default_hash
                                    , less=default_less
                                    , equal_to=default_equal_to
                                    , allocator=default_allocator )
            if defaults_eraser.normalize( cls_name ) == defaults_eraser.normalize( inst ):
                return templates.join( c_name, [defaults_eraser.erase_recursive( value_type )] )


    @staticmethod
    def erase_hashmap_compare_allocator( cls_name ):
        cls_name = defaults_eraser.replace_basic_string( cls_name )
        c_name, c_args = templates.split( cls_name )

        default_hash=None
        default_less='std::less'
        default_allocator='std::allocator'
        default_equal_to = 'std::equal_to'

        tmpl = None
        key_type = None
        mapped_type = None
        if 2 < len( c_args ):
            key_type = c_args[0]
            mapped_type = c_args[1]
        else:
            return

        if 4 == len( c_args ):
            default_hash = 'hash_compare'
            tmpl = string.Template( "$container< $key_type, $mapped_type, $hash<$key_type, $less<$key_type> >, $allocator< std::pair< const $key_type, $mapped_type> > >" )
            if key_type.startswith( 'const ' ) or key_type.endswith( ' const' ):
                tmpl = string.Template( "$container< $key_type, $mapped_type, $hash<$key_type, $less<$key_type> >, $allocator< std::pair< $key_type, $mapped_type> > >" )
        elif 5 == len( c_args ):
            default_hash = 'hash'
            tmpl = string.Template( "$container< $key_type, $mapped_type, $hash<$key_type >, $equal_to<$key_type>, $allocator< $mapped_type> >" )
            if key_type.startswith( 'const ' ) or key_type.endswith( ' const' ):
                tmpl = string.Template( "$container< $key_type, $mapped_type, $hash<$key_type >, $equal_to<$key_type>, $allocator< $mapped_type > >" )
        else:
            return

        for ns in std_namespaces:
            inst = tmpl.substitute( container=c_name
                                    , key_type=key_type
                                    , mapped_type=mapped_type
                                    , hash=ns + '::' + default_hash
                                    , less=default_less
                                    , equal_to = default_equal_to
                                    , allocator=default_allocator )
            if defaults_eraser.normalize( cls_name ) == defaults_eraser.normalize( inst ):
                return templates.join( c_name
                                       , [ defaults_eraser.erase_recursive( key_type )
                                           , defaults_eraser.erase_recursive( mapped_type )] )


class container_traits_impl_t:
    """
    implements the functionality needed for convenient work with STD container classes

    Implemented functionality:
      * find out whether a declaration is STD container or not
      * find out container value( mapped ) type

    This class tries to be useful as much, as possible. For example, for class
    declaration( and not definition ) it parsers the class name in order to
    extract the information.
    """
    def __init__( self
                  , container_name
                  , element_type_index
                  , element_type_typedef
                  , defaults_remover
                  , key_type_index=None
                  , key_type_typedef=None ):
        """
        :param container_name: std container name
        :param element_type_index: position of value\\mapped type within template arguments list
        :param element_type_typedef: class typedef to the value\\mapped type
        :param key_type_index: position of key type within template arguments list
        :param key_type_typedef: class typedef to the key type
        """
        self._name = container_name
        self.remove_defaults_impl = defaults_remover
        self.element_type_index = element_type_index
        self.element_type_typedef = element_type_typedef
        self.key_type_index = key_type_index
        self.key_type_typedef = key_type_typedef

    def name(self):
        return self._name

    def get_container_or_none( self, type_ ):
        """returns reference to the class declaration or None"""
        type_ = type_traits.remove_alias( type_ )
        type_ = type_traits.remove_cv( type_ )

        cls = None
        if isinstance( type_, cpptypes.declarated_t ):
            cls = type_traits.remove_alias( type_.declaration )
        elif isinstance( type_, class_declaration.class_t ):
            cls = type_
        elif isinstance( type_, class_declaration.class_declaration_t ):
            cls = type_
        else:
            return

        if not cls.name.startswith( self.name() + '<' ):
            return

        for ns in std_namespaces:
            if type_traits.impl_details.is_defined_in_xxx( ns, cls ):
                return cls

    def is_my_case( self, type_ ):
        """checks, whether type is STD container or not"""
        return bool( self.get_container_or_none( type_ ) )

    def class_declaration( self, type_ ):
        """returns reference to the class declaration"""
        cls = self.get_container_or_none( type_ )
        if not cls:
            raise TypeError( 'Type "%s" is not instantiation of std::%s' % ( type_.decl_string, self.name() ) )
        return cls

    def is_sequence( self, type_ ):
        #raise exception if type is not container
        unused = self.class_declaration( type_ )
        return self.key_type_index is None

    def is_mapping( self, type_ ):
        return not self.is_sequence( type_ )

    def __find_xxx_type( self, type_, xxx_index, xxx_typedef, cache_property_name ):
        cls = self.class_declaration( type_ )
        result = getattr( cls.cache, cache_property_name )
        if not result:
            if isinstance( cls, class_declaration.class_t ):
                xxx_type = cls.typedef( xxx_typedef, recursive=False ).type
                result = type_traits.remove_declarated( xxx_type )
            else:
                xxx_type_str = templates.args( cls.name )[xxx_index]
                result = type_traits.impl_details.find_value_type( cls.top_parent, xxx_type_str )
                if None is result:
                    raise RuntimeError( "Unable to find out %s '%s' key\\value type."
                                        % ( self.name(), cls.decl_string ) )
            setattr( cls.cache, cache_property_name, result )
        return result

    def element_type( self, type_ ):
        """returns reference to the class value\\mapped type declaration"""
        return self.__find_xxx_type( type_
                                     , self.element_type_index
                                     , self.element_type_typedef
                                     , 'container_element_type')

    def key_type( self, type_ ):
        """returns reference to the class key type declaration"""
        if not self.is_mapping( type_ ):
            raise TypeError( 'Type "%s" is not "mapping" container' % str( type_ ) )
        return self.__find_xxx_type( type_
                                     , self.key_type_index
                                     , self.key_type_typedef
                                     , 'container_key_type' )

    def remove_defaults( self, type_or_string ):
        """
        removes template defaults from a template class instantiation

        For example:
            .. code-block:: c++

               std::vector< int, std::allocator< int > >

        will become
            .. code-block:: c++

               std::vector< int >
        """
        name = type_or_string
        if not isinstance( type_or_string, str ):
            name = self.class_declaration( type_or_string ).name
        if not self.remove_defaults_impl:
            return name
        no_defaults = self.remove_defaults_impl( name )
        if not no_defaults:
            return name
        else:
            return no_defaults

create_traits = container_traits_impl_t
list_traits = create_traits( 'list'
                             , 0
                             , 'value_type'
                             , defaults_eraser.erase_allocator )

deque_traits = create_traits( 'deque'
                              , 0
                              , 'value_type'
                              , defaults_eraser.erase_allocator )

queue_traits = create_traits( 'queue'
                              , 0
                              , 'value_type'
                              , defaults_eraser.erase_container )

priority_queue_traits = create_traits( 'priority_queue'
                                       , 0
                                       , 'value_type'
                                       , defaults_eraser.erase_container_compare )

vector_traits = create_traits( 'vector'
                               , 0
                               , 'value_type'
                               , defaults_eraser.erase_allocator )

stack_traits = create_traits( 'stack'
                              , 0
                              , 'value_type'
                              , defaults_eraser.erase_container )

map_traits = create_traits( 'map'
                            , 1
                            , 'mapped_type'
                            , defaults_eraser.erase_map_compare_allocator
                            , key_type_index=0
                            , key_type_typedef='key_type')

multimap_traits = create_traits( 'multimap'
                                 , 1
                                 , 'mapped_type'
                                 , defaults_eraser.erase_map_compare_allocator
                                 , key_type_index=0
                                 , key_type_typedef='key_type')


hash_map_traits = create_traits( 'hash_map'
                                 , 1
                                 , 'mapped_type'
                                 , defaults_eraser.erase_hashmap_compare_allocator
                                 , key_type_index=0
                                 , key_type_typedef='key_type')


hash_multimap_traits = create_traits( 'hash_multimap'
                                      , 1
                                      , 'mapped_type'
                                      , defaults_eraser.erase_hashmap_compare_allocator
                                      , key_type_index=0
                                      , key_type_typedef='key_type')

set_traits = create_traits( 'set'
                            , 0
                            , 'value_type'
                            , defaults_eraser.erase_compare_allocator)

multiset_traits = create_traits( 'multiset'
                                 , 0
                                 , 'value_type'
                                 , defaults_eraser.erase_compare_allocator )

hash_set_traits = create_traits( 'hash_set'
                                 , 0
                                 , 'value_type'
                                 , defaults_eraser.erase_hash_allocator )

hash_multiset_traits = create_traits( 'hash_multiset'
                                      , 0
                                      , 'value_type'
                                      , defaults_eraser.erase_hash_allocator )

container_traits = (
      list_traits
    , deque_traits
    , queue_traits
    , priority_queue_traits
    , vector_traits
    , stack_traits
    , map_traits
    , multimap_traits
    , hash_map_traits
    , hash_multimap_traits
    , set_traits
    , hash_set_traits
    , multiset_traits
    , hash_multiset_traits )
"""tuple of all STD container traits classes"""

def find_container_traits( cls_or_string ):
    if isinstance( cls_or_string, str ):
        if not templates.is_instantiation( cls_or_string ):
            return None
        name = templates.name( cls_or_string )
        if name.startswith( 'std::' ):
            name = name[ len( 'std::' ): ]
        for cls_traits in container_traits:
            if cls_traits.name() == name:
                return cls_traits
    else:
        for cls_traits in container_traits:
            if cls_traits.is_my_case( cls_or_string ):
                return cls_traits


