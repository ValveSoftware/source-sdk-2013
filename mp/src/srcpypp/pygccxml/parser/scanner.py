# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import types
import pprint
import xml.sax
import warnings
import xml.sax.handler
from pygccxml.declarations import *
from pygccxml import utils

##convention
#XML_NN - XML Node Name
#XML_AN - XML Attribute Name
#also those constants are sorted for easy searching.
XML_AN_ABSTRACT = "abstract"
XML_AN_ACCESS = "access"
XML_AN_ALIGN = "align"
XML_AN_ARTIFICIAL = "artificial"
XML_AN_ATTRIBUTES = "attributes"
XML_AN_BASE_TYPE = "basetype"
XML_AN_BASES = "bases"
XML_AN_BITS = "bits"
XML_AN_CONST = "const"
XML_AN_CONTEXT = "context"
XML_AN_CVS_REVISION = "cvs_revision"
XML_AN_DEFAULT = "default"
XML_AN_DEMANGLED = "demangled"
XML_AN_EXPLICIT = "explicit"
XML_AN_EXTERN = "extern"
XML_AN_FILE = "file"
XML_AN_ID = "id"
XML_AN_INCOMPLETE = "incomplete"
XML_AN_INIT = "init"
XML_AN_INLINE = "inline"
XML_AN_LINE = "line"
XML_AN_MANGLED = "mangled"
XML_AN_MAX = "max"
XML_AN_MEMBERS = "members"
XML_AN_MUTABLE = "mutable"
XML_AN_NAME = "name"
XML_AN_OFFSET = "offset"
XML_AN_PURE_VIRTUAL = "pure_virtual"
XML_AN_RESTRICT = "restrict"
XML_AN_RETURNS = "returns"
XML_AN_SIZE = "size"
XML_AN_STATIC = "static"
XML_AN_THROW = "throw"
XML_AN_TYPE = "type"
XML_AN_VIRTUAL = "virtual"
XML_AN_VOLATILE = "volatile"
XML_NN_ARGUMENT = "Argument"
XML_NN_ARRAY_TYPE = "ArrayType"
XML_NN_CASTING_OPERATOR = "Converter"
XML_NN_CLASS = "Class"
XML_NN_CONSTRUCTOR = "Constructor"
XML_NN_CV_QUALIFIED_TYPE = "CvQualifiedType"
XML_NN_DESTRUCTOR = "Destructor"
XML_NN_ELLIPSIS = "Ellipsis"
XML_NN_ENUMERATION = "Enumeration"
XML_NN_ENUMERATION_VALUE = "EnumValue"
XML_NN_FIELD = "Field"
XML_NN_FILE = "File"
XML_NN_FUNCTION = "Function"
XML_NN_FUNCTION_TYPE = "FunctionType"
XML_NN_FUNDAMENTAL_TYPE = "FundamentalType"
XML_NN_FREE_OPERATOR = "OperatorFunction"
XML_NN_GCC_XML = "GCC_XML"
XML_NN_MEMBER_OPERATOR = "OperatorMethod"
XML_NN_METHOD = "Method"
XML_NN_METHOD_TYPE = "MethodType"
XML_NN_NAMESPACE = "Namespace"
XML_NN_OFFSET_TYPE = "OffsetType"
XML_NN_POINTER_TYPE = "PointerType"
XML_NN_REFERENCE_TYPE = "ReferenceType"
XML_NN_ROOT = "GCC_XML"
XML_NN_STRUCT = "Struct"
XML_NN_TYPEDEF = "Typedef"
XML_NN_UNION = "Union"
XML_NN_VARIABLE = "Variable"

class scanner_t( xml.sax.handler.ContentHandler ):
    def __init__(self, gccxml_file, decl_factory, *args ):
        xml.sax.handler.ContentHandler.__init__(self, *args )
        self.logger = utils.loggers.gccxml
        self.gccxml_file = gccxml_file
        #defining parsing tables
        self.__readers = {
               XML_NN_FILE : self.__read_file
               , XML_NN_NAMESPACE : self.__read_namespace
               , XML_NN_ENUMERATION : self.__read_enumeration
               , XML_NN_ENUMERATION_VALUE : self.__read_enumeration_value
               , XML_NN_ARRAY_TYPE : self.__read_array_type
               , XML_NN_CV_QUALIFIED_TYPE : self.__read_cv_qualified_type
               , XML_NN_POINTER_TYPE : self.__read_pointer_type
               , XML_NN_REFERENCE_TYPE : self.__read_reference_type
               , XML_NN_FUNDAMENTAL_TYPE : self.__read_fundamental_type
               , XML_NN_ARGUMENT : self.__read_argument
               , XML_NN_FUNCTION_TYPE : self.__read_function_type
               , XML_NN_METHOD_TYPE : self.__read_method_type
               , XML_NN_OFFSET_TYPE : self.__read_offset_type
               , XML_NN_TYPEDEF : self.__read_typedef
               , XML_NN_VARIABLE : self.__read_variable
               , XML_NN_CLASS : self.__read_class
               , XML_NN_STRUCT : self.__read_struct
               , XML_NN_UNION : self.__read_union
               , XML_NN_FIELD : self.__read_field
               , XML_NN_CASTING_OPERATOR : self.__read_casting_operator
               , XML_NN_CONSTRUCTOR : self.__read_constructor
               , XML_NN_DESTRUCTOR : self.__read_destructor
               , XML_NN_FUNCTION : self.__read_function
               , XML_NN_FREE_OPERATOR : self.__read_free_operator
               , XML_NN_MEMBER_OPERATOR : self.__read_member_operator
               , XML_NN_METHOD : self.__read_method
               , XML_NN_GCC_XML : self.__read_version
               , XML_NN_ELLIPSIS : self.__read_ellipsis
        }
        self.deep_declarations = [
            XML_NN_CASTING_OPERATOR
            , XML_NN_CONSTRUCTOR
            , XML_NN_DESTRUCTOR
            , XML_NN_ENUMERATION
            , XML_NN_FILE
            , XML_NN_FUNCTION
            , XML_NN_FREE_OPERATOR
            , XML_NN_MEMBER_OPERATOR
            , XML_NN_METHOD
            , XML_NN_FUNCTION_TYPE
            , XML_NN_METHOD_TYPE
        ]

        assert isinstance( decl_factory, decl_factory_t )
        self.__decl_factory = decl_factory

        #mapping from id -> decl
        self.__declarations = {}
        #list of all read declarations
        self.__calldefs = []
        #list of enums I need later
        self.__enums = []
        #mapping from id -> type
        self.__types = {}
        #mapping from id -> file
        self.__files = {}
        #mapping between decl id -> access
        self.__access = {}
        #current object under construction
        self.__inst = None
        #mapping from id to members
        self.__members = {}
        self.__compiler = None

        self.__mangled_suffix = ' *INTERNAL* '
        self.__mangled_suffix_len = len( self.__mangled_suffix )

    def read( self ):
        xml.sax.parse( self.gccxml_file, self )

    def endDocument( self ):
        #updating membership
        members_mapping = {}
        for gccxml_id, members in self.__members.items():
            decl = self.__declarations.get( gccxml_id, None )
            if not decl or not isinstance( decl, scopedef_t):
                continue
            members_mapping[ id( decl ) ] = members
        self.__members = members_mapping

    def declarations(self):
        return self.__declarations

    def calldefs( self ):
        return self.__calldefs

    def enums(self):
        return self.__enums

    def types(self):
        return self.__types

    def files(self):
        return self.__files

    def access(self):
        return self.__access

    def members(self):
        return self.__members

    def startElementNS(self, name, qname, attrs):
        return self.startElement( name[1], attrs )

    def endElementNS(self, name, qname):
        return self.endElement( name[1] )

    def startElement(self, name, attrs):
        try:
            if name not in self.__readers:
                return
            obj = self.__readers[name]( attrs )
            if not obj:
                return #it means that we worked on internals
                       #for example EnumValue of function argument
            if name in self.deep_declarations:
                self.__inst = obj
            self.__read_access( attrs )
            element_id = attrs.get(XML_AN_ID, None)
            if isinstance( obj, declaration_t ):
                obj.compiler = self.__compiler
                self.__update_membership( attrs )
                self.__declarations[ element_id ] = obj
                if not isinstance( obj, namespace_t ):
                    self.__read_location( obj, attrs )
                if isinstance( obj, class_t):
                    self.__read_bases( obj, attrs )
                self.__read_artificial(obj, attrs)
                self.__read_mangled( obj, attrs)
                self.__read_demangled( obj, attrs)
                self.__read_attributes(obj, attrs)

            elif isinstance( obj, type_t ):
                self.__types[ element_id ] = obj
                self.__read_byte_size(obj, attrs)
                self.__read_byte_align(obj, attrs)
            elif isinstance( obj, str ):
                self.__files[ element_id ] = obj
            else:
                self.logger.warning( 'Unknown object type has been found.'
                                     + ' Please report this bug to pygccxml development team.' )
        except Exception as error:
            msg = 'error occured, while parsing element with name "%s" and attrs "%s".'
            msg = msg + os.linesep + 'Error: %s.' % str( error )
            self.logger.error( msg % ( name, pprint.pformat( list(attrs.keys()) ) ) )
            raise

    def endElement(self, name):
        if name in self.deep_declarations:
            self.__inst = None

    def __read_location(self, decl, attrs):
        decl.location = location_t( file_name=attrs[XML_AN_FILE], line=int(attrs[XML_AN_LINE]))

    def __update_membership(self, attrs):
        parent = attrs.get( XML_AN_CONTEXT, None )
        if not parent:
            return
        if parent not in self.__members:
            self.__members[ parent ] = []
        self.__members[parent].append( attrs[XML_AN_ID] )

    def __read_members(self, decl, attrs ):
        decl.declarations = attrs.get(XML_AN_MEMBERS, "")

    def __read_bases(self, decl, attrs ):
        decl.bases = attrs.get( XML_AN_BASES, "" )

    def __read_artificial( self, decl, attrs ):
        decl.is_artificial = attrs.get( XML_AN_ARTIFICIAL, False )

    def __read_mangled( self, decl, attrs ):
        mangled = attrs.get( XML_AN_MANGLED, None )
        #the following patch is defined here for performance reasons
        if isinstance( mangled, bytes ) and mangled.endswith( self.__mangled_suffix ):
            mangled = mangled[:self.__mangled_suffix_len]
        decl.mangled = mangled

    def __read_demangled( self, decl, attrs ):
        decl.demangled = attrs.get( XML_AN_DEMANGLED, None )

    def __read_attributes( self, decl, attrs ):
        decl.attributes = attrs.get( XML_AN_ATTRIBUTES, None )

    def __read_access( self, attrs ):
        self.__access[ attrs[XML_AN_ID] ] = attrs.get( XML_AN_ACCESS, ACCESS_TYPES.PUBLIC )

    def __read_byte_size (self, decl, attrs):
        "Using duck typing to set the size instead of in constructor"
        size = attrs.get(XML_AN_SIZE, 0)
        decl.byte_size = int(size)/8 # Make sure the size is in bytes instead of bits

    def __read_byte_offset (self, decl, attrs):
        "Using duck typing to set the offset instead of in constructor"
        offset = attrs.get(XML_AN_OFFSET, 0)
        decl.byte_offset = int(offset)/8 # Make sure the size is in bytes instead of bits

    def __read_byte_align (self, decl, attrs):
        "Using duck typing to set the alignment"
        align = attrs.get(XML_AN_ALIGN, 0)
        decl.byte_align = int(align)/8 # Make sure the size is in bytes instead of bits

    def __read_root(self, attrs):
        pass

    def __read_file( self, attrs ):
        return attrs.get( XML_AN_NAME, '' )

    def __read_namespace(self, attrs):
        ns_name = attrs.get( XML_AN_NAME, '' )
        if '.' in ns_name:
            #if '.' in namespace then this is mangled namespace -> in c++ namespace{...}
            #that is almost true: gcc mangale name using top file name.
            #almost all files has '.' in name
            ns_name = ''
        return self.__decl_factory.create_namespace( name=ns_name )

    def __read_enumeration(self, attrs):
        enum_name = attrs.get( XML_AN_NAME, '' )
        if '$_' in enum_name or '._' in enum_name:
            #it means that this is unnamed enum. in c++ enum{ x };
            enum_name = ''
        decl = self.__decl_factory.create_enumeration( name=enum_name )
        self.__read_byte_size(decl, attrs)
        self.__read_byte_align(decl, attrs)
        self.__enums.append( decl )
        return decl

    def __read_enumeration_value( self, attrs ):
        name = attrs.get( XML_AN_NAME, '' )
        num = int(attrs[XML_AN_INIT])
        self.__inst.append_value(name, num)

    def __guess_int_value( self, value_as_str ):
        #returns instance of int or None
        #if gcc compiled the code, than it is correct!
        numeric_suffix_letters = 'UuLlFf'
        for s in numeric_suffix_letters:
            value_as_str = value_as_str.replace( s, '' )
        try:
            return int( value_as_str )
        except ValueError:
            try:
                return int( value_as_str, 16 )
            except ValueError:
                return None

    def __read_array_type( self, attrs ):
        type_ = attrs[ XML_AN_TYPE ]
        size = self.__guess_int_value( attrs.get(XML_AN_MAX, '' ) )
        if size is None:
            size = array_t.SIZE_UNKNOWN
            #The following warning is pretty useless, as it cant say what the
            #problematic declaration is.
            #msg = 'unable to find out array size from expression "%s"' % attrs[ XML_AN_MAX ]
            #warnings.warn( msg )
        return array_t( type_, size + 1 )

    def __read_cv_qualified_type( self, attrs ):
        if XML_AN_CONST in attrs and XML_AN_VOLATILE in attrs:
            return volatile_t( const_t( attrs[XML_AN_TYPE] ) )
        elif XML_AN_CONST in attrs:
            return const_t( attrs[XML_AN_TYPE] )
        elif XML_AN_VOLATILE in attrs:
            return volatile_t( attrs[XML_AN_TYPE] )
        elif XML_AN_RESTRICT in attrs:
            return restrict_t( attrs[XML_AN_TYPE] )
        else:
            assert 0

    def __read_pointer_type( self, attrs ):
        return pointer_t( attrs[XML_AN_TYPE] )

    def __read_reference_type( self, attrs ):
        return reference_t( attrs[XML_AN_TYPE] )

    def __read_fundamental_type(self, attrs ):
        try:
            return FUNDAMENTAL_TYPES[ attrs.get( XML_AN_NAME, '' ) ]
        except KeyError:
            raise RuntimeError( "pygccxml error: unable to find fundamental type with name '%s'."
                                % attrs.get( XML_AN_NAME, '' ) )

    def __read_offset_type( self,attrs ):
        base = attrs[ XML_AN_BASE_TYPE ]
        type_ = attrs[ XML_AN_TYPE ]
        if '0.9' in self.__compiler:
            return pointer_t( member_variable_type_t( class_inst=base, variable_type=type_ ) )
        else:
            return member_variable_type_t( class_inst=base, variable_type=type_ )

    def __read_argument( self, attrs ):
        if isinstance( self.__inst, calldef_type_t ):
            self.__inst.arguments_types.append( attrs[XML_AN_TYPE] )
        else:
            argument = argument_t()
            argument.name = attrs.get( XML_AN_NAME, 'arg%d' % len(self.__inst.arguments) )
            argument.type = attrs[XML_AN_TYPE]
            argument.default_value = attrs.get( XML_AN_DEFAULT, None )
            self.__read_attributes( argument, attrs )
            if argument.default_value == '<gccxml-cast-expr>':
                argument.default_value = None
            self.__inst.arguments.append( argument )

    def __read_ellipsis( self, attrs ):
        if isinstance( self.__inst, calldef_type_t ):
            self.__inst.arguments_types.append( '...' )
        else:
            argument = argument_t( type='...' )
            self.__inst.arguments.append( argument )

    def __read_calldef( self, calldef, attrs, is_declaration ):
        #destructor for example doesn't have return type
        calldef.return_type =  attrs.get( XML_AN_RETURNS, None )
        if is_declaration:
            self.__calldefs.append( calldef )
            calldef.name = attrs.get(XML_AN_NAME, '')
            calldef.has_extern = attrs.get( XML_AN_EXTERN, False )
            calldef.has_inline = bool( attrs.get( XML_AN_INLINE, "" ) == "1" )
            throw_stmt = attrs.get( XML_AN_THROW, None )
            if None is throw_stmt:
                calldef.does_throw = True
                calldef.exceptions = []
            elif "" == throw_stmt:
                calldef.does_throw = False
                calldef.exceptions = []
            else:
                calldef.does_throw = True
                calldef.exceptions = throw_stmt.split()

    def __read_member_function( self, calldef, attrs, is_declaration ):
        self.__read_calldef( calldef, attrs, is_declaration )
        calldef.has_const = attrs.get( XML_AN_CONST, False )
        if is_declaration:
            calldef.has_static = attrs.get( XML_AN_STATIC, False )
            if XML_AN_PURE_VIRTUAL in attrs:
                calldef.virtuality = VIRTUALITY_TYPES.PURE_VIRTUAL
            elif XML_AN_VIRTUAL in attrs:
                calldef.virtuality = VIRTUALITY_TYPES.VIRTUAL
            else:
                calldef.virtuality = VIRTUALITY_TYPES.NOT_VIRTUAL
        else:
            calldef.class_inst = attrs[XML_AN_BASE_TYPE]

    def __read_function_type(self, attrs):
        answer = free_function_type_t()
        self.__read_calldef( answer, attrs, False )
        return answer

    def __read_method_type(self, attrs):
        answer = member_function_type_t()
        self.__read_member_function( answer, attrs, False )
        return answer

    def __read_typedef(self, attrs ):
        return self.__decl_factory.create_typedef( name=attrs.get( XML_AN_NAME, '' ), type=attrs[XML_AN_TYPE])

    def __read_variable(self, attrs ):
        type_qualifiers = type_qualifiers_t()
        type_qualifiers.has_mutable = attrs.get(XML_AN_MUTABLE, False)
        type_qualifiers.has_static = attrs.get(XML_AN_EXTERN, False)
        bits = attrs.get( XML_AN_BITS, None )
        if bits:
            bits = int( bits )
        decl = self.__decl_factory.create_variable( name=attrs.get( XML_AN_NAME, '' )
                                                    , type=attrs[XML_AN_TYPE]
                                                    , type_qualifiers=type_qualifiers
                                                    , value=attrs.get( XML_AN_INIT, None )
                                                    , bits=bits)
        self.__read_byte_offset(decl, attrs)
        return decl

    __read_field = __read_variable #just a synonim

    def __read_class_impl(self, class_type, attrs):
        decl = None
        name = attrs.get(XML_AN_NAME, '')
        if '$' in name or '.' in name:
            name = ''
        if XML_AN_INCOMPLETE in attrs:
            decl = self.__decl_factory.create_class_declaration(name=name)
        else:
            decl = self.__decl_factory.create_class( name=name, class_type=class_type )
            if attrs.get( XML_AN_ABSTRACT, False ):
                decl.is_abstract = True
            else:
                decl.is_abstract = False
        self.__read_byte_size(decl, attrs)
        self.__read_byte_align(decl, attrs)
        return decl

    def __read_class( self, attrs ):
        return self.__read_class_impl( CLASS_TYPES.CLASS, attrs )

    def __read_struct( self, attrs ):
        return self.__read_class_impl( CLASS_TYPES.STRUCT, attrs )

    def __read_union( self, attrs ):
        return self.__read_class_impl( CLASS_TYPES.UNION, attrs )

    def __read_casting_operator(self, attrs ):
        operator = self.__decl_factory.create_casting_operator()
        self.__read_member_function( operator, attrs, True )
        return operator

    def __read_constructor( self, attrs ):
        constructor = self.__decl_factory.create_constructor()
        self.__read_member_function( constructor, attrs, True )
        constructor.explicit = attrs.get( XML_AN_EXPLICIT, False )
        return constructor

    def __read_function(self, attrs):
        gfunction = self.__decl_factory.create_free_function()
        self.__read_calldef( gfunction, attrs, True )
        return gfunction

    def __read_method(self, attrs):
        mfunction = self.__decl_factory.create_member_function()
        self.__read_member_function( mfunction, attrs, True )
        return mfunction

    def __read_destructor(self, attrs):
        destructor = self.__decl_factory.create_destructor()
        self.__read_member_function( destructor, attrs, True )
        destructor.name = '~' + destructor.name
        return destructor

    def __read_free_operator(self, attrs ):
        operator = self.__decl_factory.create_free_operator()
        self.__read_member_function( operator, attrs, True )
        if 'new' in operator.name or 'delete' in operator.name:
            operator.name = 'operator ' + operator.name
        else:
            operator.name = 'operator' + operator.name
        return operator

    def __read_member_operator(self, attrs):
        operator = self.__decl_factory.create_member_operator()
        self.__read_member_function( operator, attrs, True )
        if 'new' in operator.name or 'delete' in operator.name:
            operator.name = 'operator ' + operator.name
        else:
            operator.name = 'operator' + operator.name
        return operator

    def __read_version(self, attrs):
        logger = utils.loggers.cxx_parser
        version_str = attrs.get(XML_AN_CVS_REVISION, 0.6)
        version = float( version_str )
        if version is None:
            logger.info ( 'GCCXML version - 0.6' )
            self.__compiler = compilers.GCC_XML_06
        elif version <= 1.114:
            logger.info ( 'GCCXML version - 0.7' )
            self.__compiler = compilers.GCC_XML_07
        elif 1.115 <= version <= 1.126:
            logger.info ( 'GCCXML version - 0.9 BUGGY( %s )', version_str )
            self.__compiler = compilers.GCC_XML_09_BUGGY
        else:
            logger.info ( 'GCCXML version - 0.9( %s )', version_str )
            self.__compiler = compilers.GCC_XML_09

