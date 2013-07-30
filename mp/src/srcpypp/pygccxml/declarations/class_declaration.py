# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines classes, that describes C++ classes

This modules contains definition for next C++ declarations:
    - class definition
    - class declaration
    - small helper class for describing C++ class hierarchy
"""

from . import scopedef
from . import compilers
from . import algorithm
from . import declaration
from . import dependencies
from pygccxml import utils

class ACCESS_TYPES:
    """class that defines "access" constants"""
    PUBLIC = "public"
    PRIVATE = "private"
    PROTECTED = "protected"
    ALL = [ PUBLIC, PRIVATE, PROTECTED ]

class CLASS_TYPES:
    """class that defines "class" type constants"""
    CLASS = "class"
    STRUCT = "struct"
    UNION = "union"
    ALL = [ CLASS, STRUCT, UNION ]

def get_partial_name( name ):
    from . import templates
    from . import container_traits #prevent cyclic dependencies
    ct = container_traits.find_container_traits( name )
    if ct:
        return ct.remove_defaults( name )
    elif templates.is_instantiation( name ):
        tmpl_name, args = templates.split( name )
        for i, arg_name in enumerate( args ):
            args[i] = get_partial_name( arg_name.strip() )
        return templates.join( tmpl_name, args )
    else:
        return name


class hierarchy_info_t( object ):
    """describes class relationship"""
    def __init__(self, related_class=None, access=None, is_virtual=False ):
        """creates class that contains partial information about class relationship"""
        if related_class:
            assert( isinstance( related_class, class_t ) )
        self._related_class = related_class
        if access:
            assert( access in ACCESS_TYPES.ALL)
        self._access=access
        self._is_virtual = is_virtual

    def __eq__(self, other):
        if not isinstance( other, hierarchy_info_t ):
            return False
        return algorithm.declaration_path( self.related_class ) == algorithm.declaration_path( other.related_class ) \
               and self.access == other.access \
               and self.is_virtual == other.is_virtual

    def __hash__(self):
        return hash(algorithm.declaration_path(self.related_class))

    def __ne__( self, other):
        return not self.__eq__( other )

    def __lt__(self, other):
        if not isinstance( other, self.__class__ ):
            return self.__class__.__name__ < other.__class__.__name__
        return ( algorithm.declaration_path( self.related_class ), self.access, self.is_virtual  ) \
               < ( algorithm.declaration_path( other.related_class ), other.access, self.is_virtual )

    def _get_related_class(self):
        return self._related_class
    def _set_related_class(self, new_related_class):
        if new_related_class:
            assert( isinstance( new_related_class, class_t ) )
        self._related_class = new_related_class
    related_class = property( _get_related_class, _set_related_class
                              , doc="reference to base or derived :class:`class <class_t>`")

    def _get_access(self):
        return self._access
    def _set_access(self, new_access):
        assert( new_access in ACCESS_TYPES.ALL )
        self._access = new_access
    access = property( _get_access, _set_access )
    access_type = property( _get_access, _set_access
                            , doc="describes :class:`hierarchy type <ACCESS_TYPES>`")

    #TODO: check whether GCC XML support this and if so parser this information
    def _get_is_virtual(self):
        return self._is_virtual
    def _set_is_virtual(self, new_is_virtual):
        self._is_virtual = new_is_virtual
    is_virtual = property( _get_is_virtual, _set_is_virtual
                           , doc="indicates whether the inheritance is virtual or not")


class class_declaration_t( declaration.declaration_t ):
    """describes class declaration"""
    def __init__( self, name='' ):
        """creates class that describes C++ class declaration( and not definition )"""
        declaration.declaration_t.__init__( self, name )
        self._aliases = []
        self._container_traits = None
        self._container_traits_set = False

    def _get__cmp__items(self):
        """implementation details"""
        return []

    def i_depend_on_them( self, recursive=True ):
        return []

    def _get_aliases(self):
        return self._aliases
    def _set_aliases( self, new_aliases ):
        self._aliases = new_aliases
    aliases = property( _get_aliases, _set_aliases
                         , doc="List of :class:`aliases <typedef_t>` to this instance")

    @property
    def container_traits( self ):
        """reference to :class:`container_traits_impl_t` or None"""
        if self._container_traits_set == False:
            from . import container_traits #prevent cyclic dependencies
            self._container_traits_set = True
            self._container_traits = container_traits.find_container_traits( self )
        return self._container_traits

    def _get_partial_name_impl( self ):
        return get_partial_name( self.name )

class class_t( scopedef.scopedef_t ):
    """describes class definition"""

    USE_DEMANGLED_AS_NAME = True
    def __init__( self, name='', class_type=CLASS_TYPES.CLASS, is_abstract=False ):
        """creates class that describes C++ class definition"""
        scopedef.scopedef_t.__init__( self, name )
        if class_type:
            assert( class_type in CLASS_TYPES.ALL )
        self._class_type = class_type
        self._bases = []
        self._derived = []
        self._is_abstract = is_abstract
        self._public_members = []
        self._private_members = []
        self._protected_members = []
        self._aliases = []
        self._byte_size = 0
        self._byte_align = 0
        self._container_traits = None
        self._container_traits_set = False
        self._recursive_bases = None
        self._recursive_derived = None

    def _get_name_impl( self ):
        if not self._name: #class with empty name
            return self._name
        elif class_t.USE_DEMANGLED_AS_NAME and self.demangled and 'GCC' in self.compiler:
            if not self.cache.demangled_name:
                fname = algorithm.full_name( self.parent )
                if fname.startswith( '::' ) and not self.demangled.startswith( '::' ):
                    fname = fname[2:]
                if self.demangled.startswith( fname ):
                    tmp = self.demangled[ len( fname ): ] #demangled::name
                    if tmp.startswith( '::' ):
                        tmp = tmp[2:]
                    if '<' not in tmp and '<' in self._name:
                        #we have template class, but for some reason demangled
                        #name doesn't contain any template
                        #This happens for std::string class, but this breaks
                        #other cases, because this behaviour is not consistent
                        self.cache.demangled_name = self._name
                        return self.cache.demangled_name
                    else:
                        self.cache.demangled_name = tmp
                        return tmp
                else:
                    self.cache.demangled_name = self._name
                    return self._name
            else:
                return self.cache.demangled_name
        else:
            return self._name

    def __str__(self):
        name = algorithm.full_name(self)
        if name[:2]=="::":
            name = name[2:]
        return "%s [%s]"%(name, self.class_type)

    def _get__cmp__scope_items(self):
        """implementation details"""
        return [ self.class_type
                 , self._sorted_list( [ algorithm.declaration_path( base.related_class ) for base in self.bases ] )
                 , self._sorted_list( [ algorithm.declaration_path( derive.related_class ) for derive in self.derived ] )
                 , self.is_abstract
                 , self._sorted_list( self.public_members )
                 , self._sorted_list( self.private_members )
                 , self._sorted_list( self.protected_members ) ]

    def __eq__(self, other):
        if not scopedef.scopedef_t.__eq__( self, other ):
            return False
        return self.class_type == other.class_type \
               and self._sorted_list( [ algorithm.declaration_path( base.related_class ) for base in self.bases ] ) \
                   == other._sorted_list( [ algorithm.declaration_path( base.related_class ) for base in other.bases ] ) \
               and self._sorted_list( [ algorithm.declaration_path( derive.related_class ) for derive in self.derived ] ) \
                   == other._sorted_list( [ algorithm.declaration_path( derive.related_class ) for derive in other.derived ] ) \
               and self.is_abstract == other.is_abstract \
               and self._sorted_list( self.public_members ) \
                   == other._sorted_list( other.public_members ) \
               and self._sorted_list( self.private_members ) \
                   == other._sorted_list( other.private_members ) \
               and self._sorted_list( self.protected_members ) \
                   == self._sorted_list( other.protected_members )

    def __hash__(self):
        return hash(self.class_type)

    def _get_class_type(self):
        return self._class_type
    def _set_class_type( self, new_class_type):
        if new_class_type:
            assert( new_class_type in CLASS_TYPES.ALL )
        self._class_type = new_class_type
    class_type = property( _get_class_type, _set_class_type
                           , doc="describes class :class:`type <CLASS_TYPES>`")

    def _get_bases(self):
        return self._bases
    def _set_bases( self, new_bases ):
        self._bases = new_bases
    bases = property( _get_bases, _set_bases
                      , doc="list of :class:`base classes <hierarchy_info_t>`")

    @property
    def recursive_bases(self):
        """list of all :class:`base classes <hierarchy_info_t>`"""
        if self._recursive_bases is None:
            to_go = self.bases[:]
            all_bases = []
            while to_go:
                base = to_go.pop()
                if base not in all_bases:
                    all_bases.append( base )
                    to_go.extend( base.related_class.bases )
            self._recursive_bases = all_bases
        return self._recursive_bases

    def _get_derived(self):
        return self._derived
    def _set_derived( self, new_derived ):
        self._derived = new_derived
    derived = property( _get_derived, _set_derived
                        , doc="list of :class:`derived classes <hierarchy_info_t>`")

    @property
    def recursive_derived(self):
        """list of all :class:`derive classes <hierarchy_info_t>`"""
        if self._recursive_derived is None:
            to_go = self.derived[:]
            all_derived = []
            while to_go:
                derive = to_go.pop()
                if derive not in all_derived:
                    all_derived.append( derive )
                    to_go.extend( derive.related_class.derived )
            self._recursive_derived = all_derived
        return self._recursive_derived

    def _get_is_abstract(self):
        return self._is_abstract
    def _set_is_abstract( self, is_abstract ):
        self._is_abstract = is_abstract
    is_abstract = property( _get_is_abstract, _set_is_abstract
                            ,doc="describes whether class abstract or not" )

    def _get_public_members(self):
        return self._public_members
    def _set_public_members( self, new_public_members ):
        self._public_members = new_public_members
    public_members = property( _get_public_members, _set_public_members
                               , doc="list of all public :class:`members <declarationt_>`")

    def _get_private_members(self):
        return self._private_members
    def _set_private_members( self, new_private_members ):
        self._private_members = new_private_members
    private_members = property( _get_private_members, _set_private_members
                                , doc="list of all private :class:`members <declarationt_>`")

    def _get_protected_members(self):
        return self._protected_members
    def _set_protected_members( self, new_protected_members ):
        self._protected_members = new_protected_members
    protected_members = property( _get_protected_members, _set_protected_members
                                  , doc="list of all protected :class:`members <declarationt_>`" )

    def _get_aliases(self):
        return self._aliases
    def _set_aliases( self, new_aliases ):
        self._aliases = new_aliases
    aliases = property( _get_aliases, _set_aliases
                         , doc="List of :class:`aliases <typedef_t>` to this instance")

    def _get_byte_size(self):
        return self._byte_size
    def _set_byte_size( self, new_byte_size ):
        self._byte_size = new_byte_size
    byte_size = property( _get_byte_size, _set_byte_size
                          , doc="Size of this class in bytes @type: int")

    def _get_byte_align(self):
        return self._byte_align
    def _set_byte_align( self, new_byte_align ):
        self._byte_align = new_byte_align
    byte_align = property( _get_byte_align, _set_byte_align
                          , doc="Alignment of this class in bytes @type: int")

    def _get_declarations_impl(self):
        return self.get_members()

    def get_members( self, access=None):
        """
        returns list of members according to access type

        If access equals to None, then returned list will contain all members.
        You should not modify the list content, otherwise different optimization
        data will stop work and may to give you wrong results.

        :param access: describes desired members
        :type access: :class:ACCESS_TYPES

        :rtype: [ members ]
        """
        if access == ACCESS_TYPES.PUBLIC:
            return self.public_members
        elif access == ACCESS_TYPES.PROTECTED:
            return self.protected_members
        elif access == ACCESS_TYPES.PRIVATE:
            return self.private_members
        else:
            all_members = []
            all_members.extend( self.public_members )
            all_members.extend( self.protected_members )
            all_members.extend( self.private_members )
            return all_members

    def adopt_declaration( self, decl, access ):
        """adds new declaration to the class

        :param decl: reference to a :class:`declaration_t`

        :param access: member access type
        :type access: :class:ACCESS_TYPES
        """
        if access == ACCESS_TYPES.PUBLIC:
            self.public_members.append( decl )
        elif access == ACCESS_TYPES.PROTECTED:
            self.protected_members.append( decl )
        elif access == ACCESS_TYPES.PRIVATE:
            self.private_members.append( decl )
        else:
            raise RuntimeError( "Invalid access type: %s." % access )
        decl.parent = self
        decl.cache.reset()
        decl.cache.access_type = access

    def remove_declaration( self, decl ):
        """
        removes decl from  members list

        :param decl: declaration to be removed
        :type decl: :class:`declaration_t`
        """
        container = None
        access_type = self.find_out_member_access_type( decl )
        if access_type == ACCESS_TYPES.PUBLIC:
            container = self.public_members
        elif access_type == ACCESS_TYPES.PROTECTED:
            container = self.protected_members
        else: #decl.cache.access_type == ACCESS_TYPES.PRVATE
            container = self.private_members
        del container[ container.index( decl ) ]
        decl.cache.reset()

    def find_out_member_access_type( self, member ):
        """
        returns member access type

        :param member: member of the class
        :type member: :class:`declaration_t`

        :rtype: :class:ACCESS_TYPES
        """
        assert member.parent is self
        if not member.cache.access_type:
            access_type = None
            if member in self.public_members:
                access_type = ACCESS_TYPES.PUBLIC
            elif member in self.protected_members:
                access_type = ACCESS_TYPES.PROTECTED
            elif member in self.private_members:
                access_type = ACCESS_TYPES.PRIVATE
            else:
                raise RuntimeError( "Unable to find member within internal members list." )
            member.cache.access_type = access_type
            return access_type
        else:
            return member.cache.access_type

    def __find_out_member_dependencies( self, access_type ):
        members = self.get_members( access_type )
        answer = []
        for mem in members:
            answer.extend( mem.i_depend_on_them(recursive=True) )
        member_ids = set( [id( m ) for m in members] )
        for dependency in answer:
            if id( dependency.declaration ) in member_ids:
                dependency.access_type = access_type
        return answer

    def i_depend_on_them( self, recursive=True ):
        report_dependency = lambda *args: dependencies.dependency_info_t( self, *args )

        answer = []

        for base in self.bases:
            answer.append( report_dependency( base.related_class, base.access_type, "base class" ) )

        if recursive:
            for access_type in ACCESS_TYPES.ALL:
                answer.extend( self.__find_out_member_dependencies( access_type ) )

        return answer

    @property
    def container_traits( self ):
        """reference to :class:`container_traits_impl_t` or None"""
        if self._container_traits_set == False:
            from . import container_traits #prevent cyclic dependencies
            self._container_traits_set = True
            self._container_traits = container_traits.find_container_traits( self )
        return self._container_traits

    def find_copy_constructor( self ):
        copy_ = self.constructors( lambda x: x.is_copy_constructor, recursive=False, allow_empty=True )
        if copy_:
            return copy_[0]
        else:
            return None

    def find_trivial_constructor( self ):
        trivial = self.constructors( lambda x: x.is_trivial_constructor, recursive=False, allow_empty=True )
        if trivial:
            return trivial[0]
        else:
            return None

    def _get_partial_name_impl( self ):
        from . import type_traits #prevent cyclic dependencies
        if type_traits.is_std_string( self ):
            return 'string'
        elif type_traits.is_std_wstring( self ):
            return 'wstring'
        else:
            return get_partial_name( self.name )

    def find_noncopyable_vars( self ):
        """returns list of all `noncopyable` variables"""
        from . import type_traits as tt#prevent cyclic dependencies
        logger = utils.loggers.cxx_parser
        mvars = self.vars( lambda v: not v.type_qualifiers.has_static, recursive=False, allow_empty=True )
        noncopyable_vars = []
        for mvar in mvars:
            type_ = tt.remove_reference( mvar.type )
            if tt.is_const( type_ ):
                no_const = tt.remove_const( type_ )
                if tt.is_fundamental( no_const ) or tt.is_enum( no_const):
                    logger.debug( "__contains_noncopyable_mem_var - %s - TRUE - containes const member variable - fundamental or enum" % self.decl_string )
                    noncopyable_vars.append( mvar )
                if tt.is_class( no_const ):
                    logger.debug( "__contains_noncopyable_mem_var - %s - TRUE - containes const member variable - class" % self.decl_string )
                    noncopyable_vars.append( mvar )
                if tt.is_array( no_const ):
                    logger.debug( "__contains_noncopyable_mem_var - %s - TRUE - containes const member variable - array" % self.decl_string )
                    noncopyable_vars.append( mvar )
            if tt.class_traits.is_my_case( type_ ):
                cls = tt.class_traits.get_declaration( type_ )
                if tt.is_noncopyable( cls ):
                    logger.debug( "__contains_noncopyable_mem_var - %s - TRUE - containes member variable - class that is not copyable" % self.decl_string )
                    noncopyable_vars.append( mvar )
        logger.debug( "__contains_noncopyable_mem_var - %s - false - doesn't contains noncopyable members" % self.decl_string )
        return noncopyable_vars

    @property
    def has_vtable( self ):
        """True, if class has virtual table, False otherwise"""
        from . import calldef
        return bool( self.calldefs( lambda f: isinstance( f, calldef.member_function_t ) \
                                              and f.virtuality != calldef.VIRTUALITY_TYPES.NOT_VIRTUAL
                                    , recursive=False
                                    , allow_empty=True ) )

    @property
    def top_class( self ):
        """reference to a parent class, which contains this class and defined
        within a namespace

        if this class is defined under a namespace, self will be returned"""
        curr = self
        parent = self.parent
        while isinstance( parent, class_t ):
            curr = parent
            parent = parent.parent
        return curr

class_types = ( class_t, class_declaration_t )
