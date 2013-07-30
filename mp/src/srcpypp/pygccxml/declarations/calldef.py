# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines classes, that describes "callable" declarations

This modules contains definition for next C++ declarations:
    - operator
        - member
        - free
    - function
        - member
        - free
    - constructor
    - destructor
"""
import re
from . import cpptypes
from . import algorithm
from . import templates
from . import declaration
#from . import type_traits # moved below to fix a cyclic dependency problem
from . import dependencies
from . import call_invocation

class VIRTUALITY_TYPES:
    """class that defines "virtuality" constants"""
    NOT_VIRTUAL = 'not virtual'
    VIRTUAL = 'virtual'
    PURE_VIRTUAL = 'pure virtual'
    ALL = [NOT_VIRTUAL, VIRTUAL, PURE_VIRTUAL]
#preserving backward compatebility
FUNCTION_VIRTUALITY_TYPES = VIRTUALITY_TYPES

class CALLING_CONVENTION_TYPES:
    """class that defines "calling convention" constants"""
    UNKNOWN = ''
    CDECL = 'cdecl'
    STDCALL = 'stdcall'
    THISCALL = 'thiscall'
    FASTCALL = 'fastcall'
    SYSTEM_DEFAULT = '<<<system default>>>'

    all = ( UNKNOWN, CDECL, STDCALL, THISCALL, FASTCALL, SYSTEM_DEFAULT )

    pattern = re.compile( r'.*(?:^|\s)(?:__)?(?P<cc>cdecl|stdcall|thiscall|fastcall)(?:__)?.*' )

    @staticmethod
    def extract( text, default=UNKNOWN ):
        """extracts calling convention from the text. If the calling convention could not be found, the "default"is used"""
        if not text:
            return default
        found = CALLING_CONVENTION_TYPES.pattern.match( text )
        if found:
            return found.group( 'cc' )
        else:
            return default



#First level in hierarchy of calldef
class argument_t(object):
    """
    class, that describes argument of "callable" declaration
    """

    def __init__( self, name='', type=None, default_value=None, attributes=None):
        object.__init__(self)
        self._name = name
        self._default_value = default_value
        self._type = type
        self._attributes = attributes

    def clone( self, **keywd ):
        """constructs new argument_t instance

        return argument_t( name=keywd.get( 'name', self.name )
                           , type=keywd.get( 'type', self.type )
                           , default_value=keywd.get( 'default_value', self.default_value )
                           , attributes=keywd.get( 'attributes', self.attributes ) )

        """
        return argument_t( name=keywd.get( 'name', self.name )
                           , type=keywd.get( 'type', self.type )
                           , default_value=keywd.get( 'default_value', self.default_value )
                           , attributes=keywd.get( 'attributes', self.attributes ) )

    def __str__(self):
        if self.ellipsis:
            return "..."
        else:
            if self.default_value==None:
                return "%s %s"%(self.type, self.name)
            else:
                return "%s %s=%s"%(self.type, self.name, self.default_value)

    def __eq__(self, other):
        if not isinstance( other, self.__class__ ):
            return False
        return self.name == other.name \
               and self.default_value == other.default_value \
               and self.type == other.type

    def __hash__(self, other):
        return (hash(self.__class__) ^
                hash(self.name) ^
                hash(self.default_value) ^
                hash(self.type))

    def __ne__( self, other):
        return not self.__eq__( other )

    def __lt__(self, other):
        if not isinstance( other, self.__class__ ):
            return self.__class__.__name__ < other.__class__.__name__
        return self.name < other.name \
               and self.default_value < other.default_value \
               and self.type < other.type

    def _get_name(self):
        return self._name
    def _set_name(self, name):
        self._name = name
    name = property( _get_name, _set_name
                     , doc="""Argument name.
                     @type: str""" )

    @property
    def ellipsis(self):
        """bool, if True argument represents ellipsis ( "..." ) in function definition"""
        return isinstance( self.type, cpptypes.ellipsis_t )

    def _get_default_value(self):
        return self._default_value
    def _set_default_value(self, default_value):
        self._default_value = default_value
    default_value = property( _get_default_value, _set_default_value
                              , doc="""Argument's default value or None.
                              @type: str""")

    def _get_type(self):
        return self._type
    def _set_type(self, type):
        self._type = type
    type = property( _get_type, _set_type
                     , doc="""The argument :class:`type <type_t>`""")

    def _get_attributes( self ):
        return self._attributes
    def _set_attributes( self, attributes ):
        self._attributes = attributes
    attributes = property( _get_attributes, _set_attributes
                        , doc="""GCCXML attributes, set using __attribute__((gccxml("...")))
                        @type: str
                        """ )


class calldef_t( declaration.declaration_t ):
    """base class for all "callable" declarations"""
    def __init__( self, name='', arguments=None, exceptions=None, return_type=None, has_extern=False, does_throw=True ):
        declaration.declaration_t.__init__( self, name )
        if not arguments:
            arguments = []
        self._arguments = arguments
        if not exceptions:
            exceptions = []
        self._does_throw = does_throw
        self._exceptions = exceptions
        self._return_type = return_type
        self._has_extern = has_extern
        self._demangled_name = None
        self._calling_convention = None
        self._has_inline = None

    def _get__cmp__call_items(self):
        """implementation details"""
        raise NotImplementedError()

    def _get__cmp__items( self ):
        """implementation details"""
        items = [ self.arguments
                  , self.return_type
                  , self.has_extern
                  , self.does_throw
                  , self._sorted_list( self.exceptions )
                  , self.demangled_name
                  , self.has_inline ]
        items.extend( self._get__cmp__call_items() )
        return items

    def __eq__(self, other):
        if not declaration.declaration_t.__eq__( self, other ):
            return False
        return self.return_type == other.return_type \
               and self.arguments == other.arguments \
               and self.has_extern == other.has_extern \
               and self.does_throw == other.does_throw \
               and self._sorted_list( self.exceptions ) == other._sorted_list( other.exceptions ) \
               and self.demangled_name == other.demangled_name

    def __hash__(self):
        return (super.__hash__(self) ^
                hash(self.return_type) ^
                hash(self.demangled_name))

    def _get_arguments(self):
        return self._arguments
    def _set_arguments(self, arguments):
        self._arguments = arguments
    arguments = property( _get_arguments , _set_arguments
                          , doc="""The argument list.
                          @type: list of :class:`argument_t`""")

    @property
    def has_ellipsis( self ):
        return self.arguments and self.arguments[-1].ellipsis

    @property
    def argument_types( self ):
        """list of all argument types"""
        return [ arg.type for arg in self.arguments ]

    @property
    def required_args(self):
        """list of all required arguments"""
        r_args = []
        for arg in self.arguments:
            if not arg.default_value:
                r_args.append( arg )
            else:
                break
        return r_args

    @property
    def optional_args(self):
        """list of all optional arguments, the arguments that have default value"""
        return self.arguments[ len( self.required_args ) : ]

    def _get_does_throw(self):
        return self._does_throw
    def _set_does_throw(self, does_throw):
        self._does_throw = does_throw
    does_throw = property( _get_does_throw, _set_does_throw,
                           doc="""If False, than function does not throw any exception.
                           In this case, function was declared with empty throw
                           statement.
                           """)

    def _get_exceptions(self):
        return self._exceptions
    def _set_exceptions(self, exceptions):
        self._exceptions = exceptions
    exceptions = property( _get_exceptions, _set_exceptions
                           , doc="""The list of exceptions.
                           @type: list of :class:`declaration_t`""")

    def _get_return_type(self):
        return self._return_type
    def _set_return_type(self, return_type):
        self._return_type = return_type
    return_type = property( _get_return_type, _set_return_type
                            , doc='''The type of the return value of the "callable" or None (constructors).
                            @type: :class:`type_t`
                            ''')
    @property
    def overloads(self):
        """A list of overloaded "callables" (i.e. other callables with the same name within the same scope.

        @type: list of :class:`calldef_t`
        """
        if not self.parent:
            return []
        # finding all functions with the same name
        return self.parent.calldefs( name=self.name
                                     , function=lambda decl: not (decl is self )
                                     , allow_empty=True
                                     , recursive=False )

    def _get_has_extern(self):
        return self._has_extern
    def _set_has_extern(self, has_extern):
        self._has_extern = has_extern
    has_extern = property( _get_has_extern, _set_has_extern,
                           doc="""Was this callable declared as "extern"?
                           @type: bool
                           """)

    def _get_has_inline(self):
        return self._has_inline
    def _set_has_inline(self, has_inline):
        self._has_inline = has_inline
    has_inline = property( _get_has_inline, _set_has_inline,
                           doc="""Was this callable declared with "inline" specifier
                           @type: bool
                           """)

    def __remove_parent_fname( self, demangled ):
        """implementation details"""
        demangled = demangled.strip()
        parent_fname = algorithm.full_name( self.parent )
        if parent_fname.startswith( '::' ) and not demangled.startswith( '::' ):
            parent_fname = parent_fname[2:]
        demangled = demangled[ len( parent_fname ): ]
        return demangled

    def _get_demangled_name( self ):
        from . import type_traits

        if not self.demangled:
            self._demangled_name = ''

        if self._demangled_name:
            return self._demangled_name

        if self._demangled_name == '':
            return self.name

        demangled = self.demangled
        if self.return_type:
            return_type = type_traits.remove_alias( self.return_type ).decl_string

            if return_type.startswith( '::' ) and not self.demangled.startswith( '::' ):
                return_type = return_type[2:]
            demangled = self.demangled
            if demangled.startswith( return_type ):
                demangled = demangled[ len( return_type ): ]
                demangled = demangled.strip()
        #removing scope
        demangled_name = call_invocation.name( self.__remove_parent_fname( demangled ) )
        if demangled_name.startswith( '::' ):
            demangled_name = demangled_name[2:]
        #to be on the safe side
        if demangled_name.startswith( self.name ):
            self._demangled_name = demangled_name
            return self._demangled_name

        #well, I am going to try an other strategy
        fname = algorithm.full_name( self )
        found = self.demangled.find( fname )
        if -1 == found:
            if fname.startswith( '::' ):
                fname = fname[2:]
            found = self.demangled.find( fname )
            if -1 == found:
                self._demangled_name = ''
                return self.name
        demangled_name = call_invocation.name( self.demangled[ found: ] )
        demangled_name = self.__remove_parent_fname( demangled_name )
        if demangled_name.startswith( '::' ):
            demangled_name = demangled_name[2:]
        #to be on the safe side
        if demangled_name.startswith( self.name ):
            self._demangled_name = demangled_name
            return self._demangled_name
        #if -1 == found:
        self._demangled_name = ''
        return self.name

    demangled_name = property( _get_demangled_name
                              , doc="returns function demangled name. It can help you to deal with function template instantiations")

    def i_depend_on_them( self, recursive=True ):
        report_dependency = lambda *args, **keywd: dependencies.dependency_info_t( self, *args, **keywd )
        answer = []
        if self.return_type:
            answer.append( report_dependency( self.return_type, hint="return type" ) )
        for arg in self.arguments:
            answer.append( report_dependency( arg.type ) )
        for exc in self.exceptions:
            answer.append( report_dependency( exc, hint="exception" ) )
        return answer

    def guess_calling_convention( self ):
        """This function should be overriden in the derived classes and return
        more-or-less successfull guess about calling convention"""
        return CALLING_CONVENTION_TYPES.UNKNOWN

    def get_calling_convention( self ):
        if self._calling_convention is None:
            self._calling_convention = CALLING_CONVENTION_TYPES.extract( self.attributes )
            if not self._calling_convention:
                self._calling_convention = self.guess_calling_convention()
        return self._calling_convention

    def set_calling_convention( self, cc ):
        self._calling_convention = cc

    calling_convention = property( get_calling_convention, set_calling_convention
                                   , doc="function calling convention. See :class:CALLING_CONVENTION_TYPES class for possible values" )


#Second level in hierarchy of calldef
class member_calldef_t( calldef_t ):
    """base class for "callable" declarations that defined within C++ class or struct"""
    def __init__( self, virtuality=None, has_const=None, has_static=None, *args, **keywords ):
        calldef_t.__init__( self, *args, **keywords )
        self._virtuality = virtuality
        self._has_const = has_const
        self._has_static = has_static

    def __str__(self):
        # Get the full name of the calldef...
        name = algorithm.full_name(self)
        if name[:2]=="::":
            name = name[2:]
        # Add the arguments...
        args = [str(a) for a in self.arguments]
        res = "%s(%s)"%(name, ", ".join(args))
        # Add the return type...
        if self.return_type!=None:
            res = "%s %s"%(self.return_type, res)
        # const?
        if self.has_const:
            res += " const"
        # static?
        if self.has_static:
            res = "static "+res
        # Append the declaration class
        cls = self.__class__.__name__
        if cls[-2:]=="_t":
            cls = cls[:-2]
        cls = cls.replace( '_', ' ' )
        return "%s [%s]"%(res, cls)

    def _get__cmp__call_items(self):
        """implementation details"""
        return [ self.virtuality, self.has_static, self.has_const ]

    def __eq__(self, other):
        if not calldef_t.__eq__( self, other ):
            return False
        return self.virtuality == other.virtuality \
               and self.has_static == other.has_static \
               and self.has_const == other.has_const

    def __hash__(self): return super.__hash__(self)

    def get_virtuality(self):
        return self._virtuality
    def set_virtuality(self, virtuality):
        assert virtuality in VIRTUALITY_TYPES.ALL
        self._virtuality = virtuality
    virtuality = property( get_virtuality, set_virtuality
                           , doc="""Describes the "virtuality" of the member (as defined by the string constants in the class :class:VIRTUALITY_TYPES).
                           @type: str""")

    def _get_access_type(self):
        return self.parent.find_out_member_access_type( self )
    access_type = property( _get_access_type
                            , doc="""Return the access type of the member (as defined by the string constants in the class :class:ACCESS_TYPES.
                            @type: str""")

    def _get_has_const(self):
        return self._has_const
    def _set_has_const(self, has_const):
        self._has_const = has_const
    has_const = property( _get_has_const, _set_has_const
                          , doc="""describes, whether "callable" has const modifier or not""")

    def _get_has_static(self):
        return self._has_static
    def _set_has_static(self, has_static):
        self._has_static = has_static
    has_static = property( _get_has_static, _set_has_static
                           , doc="""describes, whether "callable" has static modifier or not""")

    def function_type(self):
        """returns function type. See :class:`type_t` hierarchy"""
        if self.has_static:
            return cpptypes.free_function_type_t( return_type=self.return_type
                                           , arguments_types=[ arg.type for arg in self.arguments ] )
        else:
            return cpptypes.member_function_type_t( class_inst=self.parent
                                           , return_type=self.return_type
                                           , arguments_types=[ arg.type for arg in self.arguments ]
                                           , has_const=self.has_const )

    def create_decl_string(self, with_defaults=True):
        f_type = self.function_type()
        if with_defaults:
            return f_type.decl_string
        else:
            return f_type.partial_decl_string

    def guess_calling_convention( self ):
        if self.has_static:
            return CALLING_CONVENTION_TYPES.SYSTEM_DEFAULT
        else:
            return CALLING_CONVENTION_TYPES.THISCALL

class free_calldef_t( calldef_t ):
    """base class for "callable" declarations that defined within C++ namespace"""
    def __init__( self, *args, **keywords ):
        calldef_t.__init__( self, *args, **keywords )

    def __str__(self):
        # Get the full name of the calldef...
        name = algorithm.full_name(self)
        if name[:2]=="::":
            name = name[2:]
        # Add the arguments...
        args = [str(a) for a in self.arguments]
        res = "%s(%s)"%(name, ", ".join(args))
        # Add the return type...
        if self.return_type!=None:
            res = "%s %s"%(self.return_type, res)
        # extern?
        if self.has_extern:
            res = "extern "+res
        # Append the declaration class
        cls = self.__class__.__name__
        if cls[-2:]=="_t":
            cls = cls[:-2]
        cls = cls.replace( '_', ' ' )
        return "%s [%s]"%(res, cls)

    def _get__cmp__call_items(self):
        """implementation details"""
        return []

    def function_type(self):
        """returns function type. See :class:`type_t` hierarchy"""
        return cpptypes.free_function_type_t( return_type=self.return_type
                                     , arguments_types=[ arg.type for arg in self.arguments ] )

    def create_decl_string(self, with_defaults=True):
        f_type = self.function_type()
        if with_defaults:
            return f_type.decl_string
        else:
            return f_type.partial_decl_string

    def guess_calling_convention( self ):
        """This function should be overriden in the derived classes and return
        more-or-less successfull guess about calling convention"""
        return CALLING_CONVENTION_TYPES.UNKNOWN


class operator_t(object):
    """base class for "operator" declarations"""
    OPERATOR_WORD_LEN = len( 'operator' )
    def __init__(self):
        object.__init__(self)

    @property
    def symbol(self):
        "operator's symbol. For example: operator+, symbol is equal to '+'"
        return self.name[operator_t.OPERATOR_WORD_LEN:].strip()

#Third level in hierarchy of calldef
class member_function_t( member_calldef_t ):
    """describes member function declaration"""
    def __init__( self, *args, **keywords ):
        member_calldef_t.__init__( self, *args, **keywords )

class constructor_t( member_calldef_t ):
    """describes constructor declaration"""
    def __init__( self, *args, **keywords ):
        member_calldef_t.__init__( self, *args, **keywords )
        self._explicit = True

    def _get_explicit(self):
        return self._explicit
    def _set_explicit(self, explicit):
        if explicit in [True, '1']:
            self._explicit = True
        else:
            self._explicit = False
    explicit = property( _get_explicit, _set_explicit
                     , doc="""True, if constructor has "explicit" keyword, False otherwise
                              @type: bool""" )

    def __str__(self):
        # Get the full name of the calldef...
        name = algorithm.full_name(self)
        if name[:2]=="::":
            name = name[2:]
        # Add the arguments...
        args = [str(a) for a in self.arguments]
        res = "%s(%s)"%(name, ", ".join(args))
        # Append the declaration class
        cls = 'constructor'
        if self.is_copy_constructor:
            cls = 'copy ' + cls
        return "%s [%s]"%(res, cls)

    @property
    def is_copy_constructor(self):
        """returns True if described declaration is copy constructor, otherwise False"""
        from . import type_traits
        args = self.arguments
        if 1 != len( args ):
            return False
        arg = args[0]
        if not type_traits.is_reference( arg.type ):
            return False
        if not type_traits.is_const( arg.type.base ):
            return False
        unaliased = type_traits.remove_alias( arg.type.base )
        #unaliased now refers to const_t instance
        if not isinstance( unaliased.base, cpptypes.declarated_t ):
            return False
        return id(unaliased.base.declaration) == id(self.parent)

    @property
    def is_trivial_constructor(self):
        return not bool( self.arguments )

class destructor_t( member_calldef_t ):
    """describes deconstructor declaration"""
    def __init__( self, *args, **keywords ):
        member_calldef_t.__init__( self, *args, **keywords )

class member_operator_t( member_calldef_t, operator_t ):
    """describes member operator declaration"""
    def __init__( self, *args, **keywords ):
        member_calldef_t.__init__( self, *args, **keywords )
        operator_t.__init__( self, *args, **keywords )
        self.__class_types = None

class casting_operator_t( member_calldef_t, operator_t ):
    """describes casting operator declaration"""
    def __init__( self, *args, **keywords ):
        member_calldef_t.__init__( self, *args, **keywords )
        operator_t.__init__( self, *args, **keywords )

class free_function_t( free_calldef_t ):
    """describes free function declaration"""
    def __init__( self, *args, **keywords ):
        free_calldef_t.__init__( self, *args, **keywords )

    def get_mangled_name( self ):
        if not self._mangled and not self._demangled \
           and not '<' in self.name and not self.overloads:
           #it is possible we deal with C function, so lets put it name as mangled one
           return self.name
        else:
            return self._mangled


class free_operator_t( free_calldef_t, operator_t ):
    """describes free operator declaration"""
    def __init__( self, *args, **keywords ):
        free_calldef_t.__init__( self, *args, **keywords )
        operator_t.__init__( self, *args, **keywords )
        self.__class_types = None

    @property
    def class_types( self ):
        """list of class/class declaration types, extracted from the operator arguments"""
        from . import type_traits
        if None is self.__class_types:
            self.__class_types = []
            for type_ in self.argument_types:
                decl = None
                type_ = type_traits.remove_reference( type_ )
                if type_traits.is_class( type_ ):
                    decl = type_traits.class_traits.get_declaration( type_ )
                elif type_traits.is_class_declaration( type_ ):
                    decl = type_traits.class_declaration_traits.get_declaration( type_ )
                else:
                    pass
                if decl:
                    self.__class_types.append( decl )
        return self.__class_types
