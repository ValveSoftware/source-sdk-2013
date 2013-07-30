# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""contains classes that allow to configure code generation for free\\member functions, operators and etc."""

import os
from . import user_text
from . import algorithm
from . import decl_wrapper
from pyplusplus import messages
from pygccxml import declarations
from pyplusplus import function_transformers as ft

class calldef_t(decl_wrapper.decl_wrapper_t):
    """base class, for code generator configuration, for function declaration classes."""

    BOOST_PYTHON_MAX_ARITY = 10
    """Boost.Python configuration macro value.

    A function has more than BOOST_PYTHON_MAX_ARITY arguments, will not compile.
    You should adjust BOOST_PYTHON_MAX_ARITY macro.
    For more information see: http://mail.python.org/pipermail/c++-sig/2002-June/001554.html
    """

    def __init__(self, *arguments, **keywords):
        decl_wrapper.decl_wrapper_t.__init__( self, *arguments, **keywords )

        self._call_policies = None
        self._use_keywords = True
        self._use_default_arguments = True
        self._create_with_signature = None
        self._overridable = None
        self._non_overridable_reason = None
        self._transformations = None

    def get_call_policies(self):
        return self._call_policies
    def set_call_policies(self, call_policies):
        self._call_policies = call_policies
    call_policies = property( get_call_policies, set_call_policies
                              , doc="reference to :class:`decl_wrappers.call_policy_t` class." \
                                   +"Default value is calculated at runtime, based on return value.")

    def _get_use_keywords(self):
        return self._use_keywords and bool( self.arguments )
    def _set_use_keywords(self, use_keywords):
        self._use_keywords = use_keywords
    use_keywords = property( _get_use_keywords, _set_use_keywords
                             , doc="boolean, if True, allows to call function from Python using keyword arguments." \
                                  +"Default value is True.")

    def _get_create_with_signature(self):
        if None is self._create_with_signature:
            self._create_with_signature = bool( self.overloads )

            if not self._create_with_signature and declarations.templates.is_instantiation( self.name ):
                self._create_with_signature = True

            if not self._create_with_signature and isinstance( self.parent, declarations.class_t ):
                for hi in self.parent.recursive_bases:
                    if hi.access_type == 'private':
                        continue
                    funcs = hi.related_class.calldefs( self.name, recursive=False, allow_empty=True )
                    for f in funcs:
                        if f.argument_types != self.argument_types:
                            self._create_with_signature = True
                            break
                    if self._create_with_signature:
                        break
                if not self._create_with_signature:
                    self._create_with_signature \
                        = bool( self.parent.calldefs( self.name, recursive=False, allow_empty=True ) )
        return self._create_with_signature

    def _set_create_with_signature(self, create_with_signature):
        self._create_with_signature = create_with_signature
    create_with_signature = property( _get_create_with_signature, _set_create_with_signature
                                      , doc="boolean, if True `Py++` will generate next code: def( ..., function type( function ref )"\
                                         +"Thus, the generated code is safe, when a user creates function overloading." \
                                         +"Default value is computed, based on information from the declarations tree" )

    def _get_use_default_arguments(self):
        return self._use_default_arguments
    def _set_use_default_arguments(self, use_default_arguments):
        self._use_default_arguments = use_default_arguments
    use_default_arguments = property( _get_use_default_arguments, _set_use_default_arguments
                                      , doc="boolean, if True `Py++` will generate code that will set default arguments" \
                                           +"Default value is True.")

    def has_wrapper( self ):
        """returns True, if function - wrapper is needed

        The functionality by this function is incomplete. So please don't
        use it in your code.
        """
        if not isinstance( self, declarations.member_calldef_t ):
            return False
        elif self.virtuality == declarations.VIRTUALITY_TYPES.PURE_VIRTUAL:
            return True
        elif self.access_type == declarations.ACCESS_TYPES.PROTECTED:
            return True
        else:
            return False

    def get_overridable( self ):
        """Check if the method can be overridden."""
        if None is self._overridable:
            if isinstance( self, declarations.member_calldef_t ) \
               and self.virtuality != declarations.VIRTUALITY_TYPES.NOT_VIRTUAL \
               and declarations.is_reference( self.return_type ):
                self._overridable = False
                self._non_overridable_reason = messages.W1049
            else:
                self._overridable = True
                self._non_overridable_reason = ""
        return self._overridable

    def set_overridable( self, overridable ):
        self._overridable = overridable

    overridable = property( get_overridable, set_overridable
                            , doc = get_overridable.__doc__ )

    @property
    def non_overridable_reason( self ):
        """returns the reason the function could not be overridden"""
        return self._non_overridable_reason

    def mark_as_non_overridable( self, reason ):
        """
        mark this function as final - user will not be able to override it from Python

        Not all functions could be overridden from Python, for example virtual function
        that returns non const reference to a member variable. `Py++` allows you to
        mark these functions and provide and explanation to the user.
        """
        self.overridable = False
        self._non_overridable_reason = messages.W0000 % reason

    @property
    def transformations(self):
        """return list of function transformations that should be applied on the function"""
        if None is self._transformations:
            #TODO: for trivial cases get_size( int&, int& ) `Py++` should guess
            #function transformers
            self._transformations = []
        return self._transformations

    def add_transformation(self, *transformer_creators, **keywd):
        """add new function transformation.

        transformer_creators - list of transformer creators, which should be applied on the function
        keywd - keyword arguments for :class:`function_transformers.function_transformation_t` class initialization
        """
        self.transformations.append( ft.function_transformation_t( self, transformer_creators, **keywd ) )

    def _exportable_impl_derived( self ):
        return ''

    def _exportable_impl( self ):
        if self.transformations:
            #It is possible that the function asked for the user attention.
            #The user paid attention and created a transformation.
            #Py++ should be silent in this case.
            return '' 
        if not self.parent.name:
            return messages.W1057 % str( self )
        all_types = [ arg.type for arg in self.arguments ]
        all_types.append( self.return_type )
        for some_type in all_types:
            if isinstance( some_type, declarations.ellipsis_t ):
                return messages.W1053 % str( self )
            units = declarations.decompose_type( some_type )
            ptr2functions = [unit for unit in units if isinstance( unit, declarations.calldef_type_t )]
            if ptr2functions:
                return messages.W1004
            #Function that take as agrument some instance of non public class
            #will not be exported. Same to the return variable
            if isinstance( units[-1], declarations.declarated_t ):
                dtype = units[-1]
                if isinstance( dtype.declaration.parent, declarations.class_t ):
                    if dtype.declaration not in dtype.declaration.parent.public_members:
                        return messages.W1005
            no_ref = declarations.remove_reference( some_type )
            no_ptr = declarations.remove_pointer( no_ref )
            no_const = declarations.remove_const( no_ptr )
            if declarations.is_array( no_const ):
                return messages.W1006
        return self._exportable_impl_derived()

    def _readme_impl( self ):
        def is_double_ptr( type_ ):
            #check for X**
            if not declarations.is_pointer( type_ ):
                return False
            base = declarations.remove_pointer( type_ )
            return declarations.is_pointer( base )

        def suspicious_type( type_ ):
            if not declarations.is_reference( type_ ):
                return False
            type_no_ref = declarations.remove_reference( type_ )
            return not declarations.is_const( type_no_ref ) \
                   and ( declarations.is_fundamental( type_no_ref )
                         or declarations.is_enum( type_no_ref ) )
        msgs = []
        #TODO: functions that takes as argument pointer to pointer to smth, could not be exported
        #see http://www.boost.org/libs/python/doc/v2/faq.html#funcptr

        if len( self.arguments ) > calldef_t.BOOST_PYTHON_MAX_ARITY:
            msgs.append( messages.W1007 % ( calldef_t.BOOST_PYTHON_MAX_ARITY, len( self.arguments ) ) )

        if self.transformations:
            #if user defined transformation, than I think it took care of the problems
            ft = self.transformations[0]
            if ft.alias == ft.unique_name:
                msgs.append( messages.W1044 % ft.alias )
            return msgs

        if suspicious_type( self.return_type ) and None is self.call_policies:
            msgs.append( messages.W1008 )

        if ( declarations.is_pointer( self.return_type ) or is_double_ptr( self.return_type ) ) \
           and None is self.call_policies:
            msgs.append( messages.W1050 % str(self.return_type) )

        for index, arg in enumerate( self.arguments ):
            if suspicious_type( arg.type ):
                msgs.append( messages.W1009 % ( arg.name, index ) )
            if is_double_ptr( arg.type ):
                msgs.append( messages.W1051 % ( arg.name, index, str(arg.type) ) )

        if False == self.overridable:
            msgs.append( self._non_overridable_reason)

        problematics = algorithm.registration_order.select_problematics( self )
        if problematics:
            tmp = []
            for f in problematics:
                tmp.append( os.linesep + '\t' + str(f) )
            msgs.append( messages.W1010 % os.linesep.join( tmp ) )
        return msgs

class member_function_t( declarations.member_function_t, calldef_t ):
    """defines a set of properties, that will instruct `Py++` how to expose the member function"""
    def __init__(self, *arguments, **keywords):
        declarations.member_function_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )
        self._use_overload_macro = False
        self._override_precall_code = []
        self._overide_native_precall_code = []
        self._default_precall_code =  []
        self._adaptor = None

    def _get_adaptor(self):
        return self._adaptor
    def _set_adaptor(self, adaptor):
        self._adaptor = adaptor
    adaptor = property( _get_adaptor, _set_adaptor
                        , doc="string, if contains value `Py++` will generate code the following code: " \
                             +".def(<name>, <adaptor>(<function reference>), <other args> ) " \
                             +". The property is relevant for public, non virtual member functions." )


    def add_override_precall_code(self, code):
        """add code, which should be executed, before overridden member function call"""
        self._override_precall_code.append( code )

    @property
    def override_precall_code(self):
        """code, which should be executed, before overrided member function call"""
        return self._override_precall_code

    def add_override_native_precall_code(self, code):
        """add code, which should be executed, before native member function call"""
        self._overide_native_precall_code.append( code )

    @property
    def override_native_precall_code(self):
        """code, which should be executed, before overrided member function call"""
        return self._overide_native_precall_code

    def add_default_precall_code(self, code):
        """add code, which should be executed, before this member function call"""
        self._default_precall_code.append( code )

    @property
    def default_precall_code(self):
        """code, which should be executed, before this member function call"""
        return self._default_precall_code

    def get_use_overload_macro(self):
        return self._use_overload_macro
    def set_use_overload_macro(self, use_macro):
        self._use_overload_macro = use_macro
    use_overload_macro = property( get_use_overload_macro, set_use_overload_macro
                             , doc="boolean, if True, will use BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS macro to expose declarations" \
                                  +"Default value is False.")

    def _exportable_impl_derived(self):
        if self.access_type == declarations.ACCESS_TYPES.PRIVATE \
           and self.virtuality == declarations.VIRTUALITY_TYPES.NOT_VIRTUAL:
            return messages.W1011
        return ''

    def _readme_impl( self ):
        msgs = super( member_function_t, self )._readme_impl()
        if self.does_throw == False \
           and self.virtuality != declarations.VIRTUALITY_TYPES.NOT_VIRTUAL:
            msgs.append( messages.W1046 )
        return msgs

class constructor_t( declarations.constructor_t, calldef_t ):
    """defines a set of properties, that will instruct `Py++` how to expose the constructor"""
    def __init__(self, *arguments, **keywords):
        declarations.constructor_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )
        self._body = ''
        self._allow_implicit_conversion = True

    def _get_body(self):
        return self._body
    def _set_body(self, body):
        self._body = body
    body = property( _get_body, _set_body
                     , doc="string, class-wrapper constructor body" )

    def _exportable_impl_derived( self ):
        if self.is_artificial:
            return messages.W1012
        if self.access_type == declarations.ACCESS_TYPES.PRIVATE:
            return messages.W1013
        return ''

    def does_define_implicit_conversion( self ):
        """ returns true if the constructor can take part in implicit conversions.

        For more information see:

            * http://boost.org/libs/python/doc/v2/implicit.html#implicitly_convertible-spec
            * http://msdn2.microsoft.com/en-us/library/h1y7x448.aspx
            * http://msdn.microsoft.com/en-us/library/s2ff0fz8%28VS.100%29.aspx
        """
        if self.parent.is_abstract: #user is not able to create an instance of the class
            return False
        if self.is_copy_constructor:
            return False
        if not( len( self.arguments) and len( self.required_args ) < 2 ):
            return False
        if self.parent.find_out_member_access_type( self ) != declarations.ACCESS_TYPES.PUBLIC:
            return False
        return True

    def _get_allow_implicit_conversion(self):
        return self._allow_implicit_conversion and self.does_define_implicit_conversion()
    def _set_allow_implicit_conversion(self, allow_implicit_conversion):
        self._allow_implicit_conversion = allow_implicit_conversion
    allow_implicit_conversion = property( _get_allow_implicit_conversion, _set_allow_implicit_conversion
                     , doc="boolean, indicates whether `Py++` should generate implicitly_convertible code or not" \
                           "Default value is calculated from the constructor type." )

class destructor_t( declarations.destructor_t, calldef_t ):
    """you may ignore this class for he time being.

    In future it will contain "body" property, that will allow to insert user
    code to class-wrapper destructor.
    """
    #TODO: add body property
    def __init__(self, *arguments, **keywords):
        declarations.destructor_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )

class operators_helper:
    """helps `Py++` to deal with C++ operators"""
    inplace = [ '+=', '-=', '*=', '/=',  '%=', '>>=', '<<=', '&=', '^=', '|=' ]
    comparison = [ '==', '!=', '<', '>', '<=', '>=' ]
    non_member = [ '+', '-', '*', '/', '%', '&', '^', '|', ]
    unary = [ '!', '~', '+', '-' ]

    all = inplace + comparison + non_member + unary

    @staticmethod
    def is_supported( oper ):
        """returns True if Boost.Python support the operator"""
        if oper.symbol == '*' and len( oper.arguments ) == 0:
            #dereference does not make sense
            return False
        if oper.symbol != '<<':
            return oper.symbol in operators_helper.all

        args_len = len( oper.arguments )
        if isinstance( oper, declarations.member_operator_t ):# and args_len != 1:
            return False #Boost.Python does not support member operator<< :-(
        if isinstance( oper, declarations.free_operator_t ) and args_len != 2:
            return False
        if not declarations.is_same( oper.return_type, oper.arguments[0].type ):
            return False
        type_ = oper.return_type
        if not declarations.is_reference( type_ ):
            return False
        type_ = declarations.remove_reference( type_ )
        if declarations.is_const( type_ ):
            return False
        if args_len == 2:
            #second argument should has "T const &" type, otherwise the code will not compile
            tmp = oper.arguments[1].type
            if not declarations.is_reference( tmp ):
                return False
            tmp = declarations.remove_reference( tmp )
            if not declarations.is_const( tmp ):
                return False
        return declarations.is_std_ostream( type_ ) or declarations.is_std_wostream( type_ )

    @staticmethod
    def exportable( oper ):
        """returns True if Boost.Python or `Py++` know how to export the operator"""
        if isinstance( oper, declarations.member_operator_t ) and oper.symbol in ( '()', '[]', '=' ):
            return ''
        if not operators_helper.is_supported( oper ):
            return messages.W1014 % oper.name
        if isinstance( oper, declarations.free_operator_t ):
            #`Py++` should find out whether the relevant class is exposed to Python
            #and if not, than this operator should not be exposed too
            included = [decl for decl in oper.class_types if decl.ignore == False]
            if not included:
                return messages.W1052 % str(oper)
        return ''

    @staticmethod
    def target_class( oper ):
        """this functions returns reference to class/class declaration
        in scope of which, the operator should be exposed."""
        if isinstance( oper.parent, declarations.class_t ):
            return oper.parent
        #now we deal with free operators
        def find_class( type_ ):
            type_ = declarations.remove_reference( type_ )
            if declarations.is_class( type_ ):
                return declarations.class_traits.get_declaration( type_ )
            elif declarations.is_class_declaration( type_ ):
                return declarations.class_declaration_traits.get_declaration( type_ )
            else:
                return None

        arg_1_class = find_class( oper.arguments[0].type )
        arg_2_class = None
        if 2 == len( oper.arguments ):
            arg_2_class = find_class( oper.arguments[1].type )

        if arg_1_class:
            if declarations.is_std_ostream( arg_1_class ) or declarations.is_std_wostream( arg_1_class ):
                #in most cases users doesn't expose std::ostream class
                return arg_2_class
            else:
                return arg_1_class
        else:
            return arg_2_class


class member_operator_t( declarations.member_operator_t, calldef_t ):
    """defines a set of properties, that will instruct `Py++` how to expose the member operator"""
    def __init__(self, *arguments, **keywords):
        declarations.member_operator_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )
        self._override_precall_code = []
        self._default_precall_code =  []
        self._overide_native_precall_code = []
        
    def add_override_precall_code(self, code):
        self._override_precall_code.append( code )

    @property
    def override_precall_code(self):
        return self._override_precall_code

    def add_default_precall_code(self, code):
        self._default_precall_code.append( code )

    @property
    def default_precall_code(self):
        return self._default_precall_code

    def add_override_native_precall_code(self, code):
        """add code, which should be executed, before native member function call"""
        self._overide_native_precall_code.append( code )

    @property
    def override_native_precall_code(self):
        """code, which should be executed, before overrided member function call"""
        return self._overide_native_precall_code


    def _get_alias( self):
        alias = super( member_operator_t, self )._get_alias()
        if alias == self.name:
            if self.symbol == '()':
                alias = '__call__'
            elif self.symbol == '[]':
                alias = '__getitem__'
            elif self.symbol == '=':
                alias = 'assign'
            else:
                pass
        return alias
    alias = property( _get_alias, decl_wrapper.decl_wrapper_t._set_alias
                      , doc="Gives right alias for operator()( __call__ ) and operator[]( __getitem__ )" )

    def _exportable_impl_derived( self ):
        if self.access_type == declarations.ACCESS_TYPES.PRIVATE \
           and self.virtuality == declarations.VIRTUALITY_TYPES.NOT_VIRTUAL:
            return messages.W1015
        return operators_helper.exportable( self )

    @property
    def target_class( self ):
        return self.parent

class casting_operator_t( declarations.casting_operator_t, calldef_t ):
    """defines a set of properties, that will instruct `Py++` how to expose the casting operator"""

    def prepare_special_cases():
        """
        Creates a map of special cases ( aliases ) for casting operator.
        """
        special_cases = {}
        const_t = declarations.const_t
        pointer_t = declarations.pointer_t
        for type_ in list(declarations.FUNDAMENTAL_TYPES.values()):
            alias = None
            if declarations.is_same( type_, declarations.bool_t() ):
                alias = '__int__'
            elif declarations.is_integral( type_ ):
                if 'long' in type_.decl_string:
                    alias = '__long__'
                else:
                    alias = '__int__'
            elif declarations.is_floating_point( type_ ):
                alias = '__float__'
            else:
                continue #void
            if alias:
                special_cases[ type_ ] = alias
                special_cases[ const_t( type_ ) ] = alias
        special_cases[ pointer_t( const_t( declarations.char_t() ) ) ] = '__str__'
        std_string = '::std::basic_string<char,std::char_traits<char>,std::allocator<char> >'
        std_wstring1 = '::std::basic_string<wchar_t,std::char_traits<wchar_t>,std::allocator<wchar_t> >'
        std_wstring2 = '::std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >'
        special_cases[ std_string ] = '__str__'
        special_cases[ std_wstring1 ] = '__str__'
        special_cases[ std_wstring2 ] = '__str__'
        special_cases[ '::std::string' ] = '__str__'
        special_cases[ '::std::wstring' ] = '__str__'

        #TODO: add
        #          std::complex<SomeType> some type should be converted to double
        return special_cases

    SPECIAL_CASES = prepare_special_cases()
    #casting_member_operator_t.prepare_special_cases()

    def __init__(self, *arguments, **keywords):
        declarations.casting_operator_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )

    def _get_alias( self):
        if not self._alias or self.name == super( casting_operator_t, self )._get_alias():
            return_type = declarations.remove_alias( self.return_type )
            decl_string = return_type.decl_string
            for type_, alias in list(self.SPECIAL_CASES.items()):
                if isinstance( type_, declarations.type_t ):
                    if declarations.is_same( return_type, type_ ):
                        self._alias = alias
                        break
                else:
                    if decl_string == type_:
                        self._alias = alias
                        break
            else:
                self._alias = 'as_' + self._generate_valid_name(self.return_type.decl_string)
        return self._alias
    alias = property( _get_alias, decl_wrapper.decl_wrapper_t._set_alias
                      , doc="Gives right alias for casting operators: __int__, __long__, __str__." \
                           +"If there is no built-in type, creates as_xxx alias" )

    def _exportable_impl_derived( self ):
        if not declarations.is_fundamental( self.return_type ) and not self.has_const:
            return messages.W1016
        if self.access_type != declarations.ACCESS_TYPES.PUBLIC:
            return messages.W1017
        return ''


class free_function_t( declarations.free_function_t, calldef_t ):
    """defines a set of properties, that will instruct `Py++` how to expose the free function"""
    def __init__(self, *arguments, **keywords):
        declarations.free_function_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )
        self._use_overload_macro = False
        self._declaration_code = []
        self._adaptor = None

    def _get_adaptor(self):
        return self._adaptor
    def _set_adaptor(self, adaptor):
        self._adaptor = adaptor
    adaptor = property( _get_adaptor, _set_adaptor
                        , doc="string, if contains value `Py++` will generate code the following code: " \
                             +"def(<name>, <adaptor>(<function reference>), <other args> ) " )

    def add_declaration_code( self, code ):
        """adds the code to the declaration section"""
        self.declaration_code.append( user_text.user_text_t( code ) )

    @property
    def declaration_code( self ):
        """
        List of strings, that contains valid C++ code, that will be added to
        the same file in which the registration code for the function will be
        generated
        """
        return self._declaration_code

    def get_use_overload_macro(self):
        return self._use_overload_macro
    def set_use_overload_macro(self, use_macro):
        self._use_overload_macro = use_macro
    use_overload_macro = property( get_use_overload_macro, set_use_overload_macro
                             , doc="boolean, if True, will use BOOST_PYTHON_FUNCTION_OVERLOADS macro to expose declarations" \
                                  +"Default value is False.")


class free_operator_t( declarations.free_operator_t, calldef_t ):
    """defines a set of properties, that will instruct `Py++` how to expose the free operator"""
    def __init__(self, *arguments, **keywords):
        declarations.free_operator_t.__init__( self, *arguments, **keywords )
        calldef_t.__init__( self )
        self._target_class = None

    def _exportable_impl_derived( self ):
        return operators_helper.exportable( self )

    def get_target_class( self ):
        if self._target_class is None:
            self._target_class = operators_helper.target_class( self )
        return self._target_class

    def set_target_class( self, class_ ):
        self._target_class = class_

    _target_class_doc_ = "reference to class_t or class_declaration_t object." \
                       + " There are use cases, where `Py++` doesn't guess right, in what scope" \
                       + " free operator should be registered( exposed ). If this is your use case " \
                       + " than setting the class will allow you to quickly fix the situation. "

    target_class = property( get_target_class, set_target_class, doc=_target_class_doc_ )
