# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines class that configure global and member variable exposing"""

from . import decl_wrapper
from . import python_traits
from . import call_policies
from . import python_traits
from pyplusplus import messages
from pygccxml import declarations

class variable_t(decl_wrapper.decl_wrapper_t, declarations.variable_t):
    """defines a set of properties, that will instruct `Py++` how to expose the variable"""

    def __init__(self, *arguments, **keywords):
        declarations.variable_t.__init__(self, *arguments, **keywords )
        decl_wrapper.decl_wrapper_t.__init__( self )
        self._getter_call_policies = None
        self._setter_call_policies = None
        self._apply_smart_ptr_wa = False
        self._is_read_only = None
        self._use_make_functions = None
        self._expose_address = None
        self._expose_value = None

    __call_policies_doc__ = \
    """There are usecase, when exporting member variable forces `Py++` to
    create accessors functions. Sometime, those functions requires call policies.
    To be more specific: when you export member variable that has reference or
    pointer type, you need to tell Boost.Python library how to manage object
    life-time. In all cases, `Py++` will give reasonable default value. I am
    sure, that there are use cases, when you will have to change it. You should
    use this property to change it.
    """

    def get_getter_call_policies( self ):
        if None is self._getter_call_policies:
            if self.apply_smart_ptr_wa:
                value_policy = ''
                if self.is_read_only:
                   value_policy = call_policies.copy_const_reference
                else:
                    value_policy = call_policies.copy_non_const_reference
                self._getter_call_policies = call_policies.return_value_policy( value_policy )
            elif self.use_make_functions:
                self._getter_call_policies = call_policies.return_internal_reference()
            else:
                pass
        return self._getter_call_policies
    def set_getter_call_policies( self, call_policies ):
        self._getter_call_policies = call_policies
    getter_call_policies = property( get_getter_call_policies, set_getter_call_policies
                                     , doc=__call_policies_doc__ )

    def get_setter_call_policies( self ):
        if None is self._getter_call_policies:
            if self.apply_smart_ptr_wa or self.use_make_functions:
                self._setter_call_policies = call_policies.default_call_policies()
        return self._setter_call_policies
    def set_setter_call_policies( self, call_policies ):
        self._setter_call_policies = call_policies
    setter_call_policies = property( get_setter_call_policies, set_setter_call_policies
                                     , doc=__call_policies_doc__ )

    __use_make_functions_doc__ = \
    """
    Generate code using `make_getter` and `make_setter` functions

    Basically you don't need to use this, untill you have one of the following
    use-cases:

      * member variable is smart pointer - in this case Boost.Python has small
        problem to expose it right. Using get/set functions is a work-around.

      * member variable defined custom r-value converter - may be you don't know
        but the conversion is applied only on functions arguments. So you need to
        use make_getter/make_setter functions, allow users to enjoy from the conversion.

    Setting :attr:`apply_smart_ptr_wa` and/or :attr:`use_make_functions` to "True"
    will tell `Py++` to generate such code.
    """

    def get_apply_smart_ptr_wa( self ):
        return self._apply_smart_ptr_wa
    def set_apply_smart_ptr_wa( self, value):
        self._apply_smart_ptr_wa = value
    apply_smart_ptr_wa = property( get_apply_smart_ptr_wa, set_apply_smart_ptr_wa
                                     , doc=__use_make_functions_doc__ )

    def get_use_make_functions( self ):
        return self._use_make_functions
    def set_use_make_functions( self, value ):
        self._use_make_functions = value
    use_make_functions = property( get_use_make_functions, set_use_make_functions
                                   , doc=__use_make_functions_doc__)

    def __should_be_exposed_by_address_only(self):
        type_ = declarations.remove_alias( self.type )
        type_ = declarations.remove_const( type_ )
        type_ = declarations.remove_pointer( type_ )
        if not declarations.class_traits.is_my_case( type_ ):
            return False
        cls = declarations.class_traits.get_declaration( type_ )
        if cls.class_type == declarations.CLASS_TYPES.UNION:
            return True
        elif not cls.name:
            return True
        else:
            return False

    __expose_address_doc__ = \
    """There are some cases when Boost.Python doesn't provide a convenient way
    to expose the variable to Python. For example:

    double* x[10];
    //or
    char* buffer; //in case you want to modify the buffer in place

    In this cases `Py++` doesn't help too. In these cases it is possible to expose
    the actual address of the variable. After that, you can use built-in "ctypes"
    package to edit the content of the variable.
    """
    def get_expose_address( self ):
        if None is self._expose_address:
            self._expose_address = self.__should_be_exposed_by_address_only()
        return self._expose_address
    def set_expose_address( self, value ):
        self._expose_address = value
    expose_address = property( get_expose_address, set_expose_address
                               , doc= __expose_address_doc__ )

    __expose_value_doc__ = \
    """Boost.Python is not able to expose unions. Using ctypes module
    it is possible to get access to the data stored in a variable, which
    has some union type.

    This property controls whether `Py++` should expose the variable value
    or not. In case, this variable has type union, this property will be False.
    """
    def get_expose_value( self ):
        if None is self._expose_value:
            self._expose_value = not self.__should_be_exposed_by_address_only()
        return self._expose_value
    def set_expose_value( self, value ):
        self._expose_value = value
    expose_value = property( get_expose_value, set_expose_value
                             , doc= __expose_value_doc__ )

    def __find_out_is_read_only(self):
        type_ = declarations.remove_alias( self.type )

        if isinstance( type_, declarations.const_t ):
            return True

        if declarations.is_pointer( type_ ):
            type_ = declarations.remove_pointer( type_ )

        if declarations.is_reference( type_ ):
            type_ = declarations.remove_reference( type_ )

        if isinstance( type_, declarations.const_t ):
            return True

        if self.apply_smart_ptr_wa:
            return False #all smart pointers has assign operator

        if isinstance( type_, declarations.declarated_t ) \
           and isinstance( type_.declaration, declarations.class_t ) \
           and not declarations.has_public_assign( type_.declaration ):
            return True
        return False

    def get_is_read_only( self ):
        if None is self._is_read_only:
            self._is_read_only = self.__find_out_is_read_only()
        return self._is_read_only
    def set_is_read_only( self, v ):
        self._is_read_only = v
    is_read_only = property( get_is_read_only, set_is_read_only )

    def _exportable_impl( self ):
        if not self.parent.name and self.is_wrapper_needed():
            #return messages.W1057 % str( self )
            return messages.W1058 % str( self )
        if not self.name:
            return messages.W1033
        if self.bits == 0 and self.name == "":
            return messages.W1034
        if not self.expose_address:
            if declarations.is_array( self.type ) and declarations.array_size( self.type ) < 1:
                return messages.W1045
        type_ = declarations.remove_alias( self.type )
        type_ = declarations.remove_const( type_ )
        if declarations.is_pointer( type_ ):
            if not self.expose_address and self.type_qualifiers.has_static:
                return messages.W1035
            if not self.expose_address and python_traits.is_immutable( type_.base ):
                return messages.W1036

            units = declarations.decompose_type( type_ )
            ptr2functions = [unit for unit in units if isinstance( unit, declarations.calldef_type_t )]
            if ptr2functions:
                return messages.W1037
        type_ = declarations.remove_pointer( type_ )
        if declarations.class_traits.is_my_case( type_ ):
            cls = declarations.class_traits.get_declaration( type_ )
            if not cls.name:
                return messages.W1038
            #if cls.class_type == declarations.CLASS_TYPES.UNION:
            #    return messages.W1061 % ( str( self ), str( cls ) )
        if isinstance( self.parent, declarations.class_t ):
            if self.access_type != declarations.ACCESS_TYPES.PUBLIC:
                return messages.W1039
        if declarations.is_array( type_ ):
            item_type = declarations.array_item_type( type_ )
            if declarations.is_pointer( item_type ):
                item_type_no_ptr = declarations.remove_pointer( item_type )
                if python_traits.is_immutable( item_type_no_ptr ):
                    return messages.W1056
        return ''

    def is_wrapper_needed(self):
        """returns an explanation( list of str ) why wrapper is needed.

        If wrapper is not needed than [] will be returned.
        """
        explanation = []
        if self.bits:
            explanation.append( messages.W1024 % self.name )
        if declarations.is_pointer( self.type ):
            explanation.append( messages.W1025 % self.name )
        if declarations.is_reference( self.type ):
            explanation.append( messages.W1026 % self.name )
        if declarations.is_array( self.type ):
            explanation.append( messages.W1027 % self.name)
        return explanation
