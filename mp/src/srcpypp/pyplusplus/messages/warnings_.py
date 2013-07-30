# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""This package defines all user messages (warnings + errors), which will be
reported to user.
"""

class message_type(str):
    """implementation details"""
    def __new__(self, value, identifier=None):
        return str.__new__(self, value )

    def __init__(self, value, identifier=None):
        self.__identifier = identifier

    @property
    def identifier( self ):
        return self.__identifier

    def __mod__( self, values ):
        str_value = super( message_type, self ).__str__()
        return self.__class__( str_value % values, self.identifier )

class warning( message_type ):
    prefix = 'warning'

class compilation_error( message_type ):
    prefix = 'compilation error'

class execution_error( message_type ):
    prefix = 'execution error'

class code_generation_error( message_type ):
    prefix = 'code generation error'


W0000 = warning( '%s' ) #general message, usefull in few cases

W1000 = compilation_error(
            '`Py++`, by default, does not expose internal compilers declarations. '
            'Names of those declarations usually start with "__".' )

W1001 = compilation_error(
            '`Py++`, by default, does not expose internal declarations. '
            'GCC-XML reports that these declaration belong to "<internal>" header.' )

W1002 = compilation_error(
            '`Py++`, by default, does not expose compiler generated declarations.' )

W1003 = warning(
            'Virtual functions that returns const reference cannot be overridden from Python. '
            'Reason: boost::python::override::operator()(...) saves the result of the marshaling '
            '(from Python to C++) on the stack. Thus operator() returns reference '
            'to a temporary variable. Consider to use "Function Transformation" functionality '
            'to solve the problem.' )

W1004 = compilation_error(
            'Boost.Python library can not expose function, which takes as argument/returns '
            'pointer to function. '
            ' See http://www.boost.org/libs/python/doc/v2/faq.html#funcptr for more information.' )

W1005 = compilation_error(
            '`Py++` cannot expose function that takes as argument/returns instance of non-public class. '
            'Generated code will not compile.' )

W1006 = compilation_error(
            '`Py++` need your help to expose function that takes as argument/returns C++ arrays. '
            'Take a look on "Function Transformation" functionality and define the transformation.' )

W1007 = warning(
            'The function has more than %d arguments ( %d ). '
            'You should adjust BOOST_PYTHON_MAX_ARITY macro. '
            'For more information see: http://www.boost.org/libs/python/doc/v2/configuration.html' )

W1008 = warning(
            'The function returns non-const reference to "Python immutable" type. '
            'The value cannot be modified from Python. ' )

W1009 = execution_error(
            'The function takes as argument (name=%s, pos=%d) non-const reference '
            'to Python immutable type - function could not be called from Python. '
            'Take a look on "Function Transformation" functionality and define the transformation.' )

W1010 = execution_error(
            'The function introduces registration order problem. '
            'For more information about the problem read "registration order" document.'
            'Problematic functions list: %s' )

W1011 = warning( "`Py++` doesn't export private not virtual functions." )

W1012 = compilation_error( '`Py++` does not exports compiler generated constructors.' )

W1013 = compilation_error( "`Py++` doesn't export private constructor." )

W1014 = compilation_error(
            '"%s" is not supported. '
            'See Boost.Python documentation: http://www.boost.org/libs/python/doc/v2/operators.html#introduction.' )

W1015 = compilation_error( "`Py++` doesn't export private operators." )

W1016 = warning(
            '`Py++` does not exports non-const casting operators with user defined type as return value. '
            'This could be changed in future.' )

W1017 = compilation_error( "`Py++` doesn't export non-public casting operators." )

W1018 = compilation_error( '`Py++` can not expose anonymous class "%s", declared in a namespace.' )

W1019 = compilation_error( '`Py++` can not expose private class.' )

W1020 = warning( "`Py++` will generate class wrapper - hand written code should be added to the wrapper class" )

W1021 = warning( "`Py++` will generate class wrapper - hand written code should be added to the wrapper class null constructor body" )

W1022 = warning( "`Py++` will generate class wrapper - hand written code should be added to the wrapper class copy constructor body" )

W1023 = warning(
            "`Py++` will generate class wrapper - there are few functions that should be redefined in class wrapper. "
            "The functions are: %s." )

W1024 = warning( '`Py++` will generate class wrapper - class contains "%s" - bit field member variable' )

W1025 = warning( '`Py++` will generate class wrapper - class contains "%s" - T* member variable' )

W1026 = warning( '`Py++` will generate class wrapper - class contains "%s" - T& member variable' )

W1027 = warning( '`Py++` will generate class wrapper - class contains "%s" - array member variable' )

W1028 = warning( '`Py++` will generate class wrapper - class contains definition of nested class "%s", which requires wrapper class' )

W1029 = warning( "`Py++` will generate class wrapper - hand written code should be added to the wrapper class constructor body" )

W1030 = warning( '`Py++` will generate class wrapper - class contains "%s" - [pure] virtual member function' )

W1031 = warning( '`Py++` will generate class wrapper - user asked to expose non - public member function "%s"' )

W1032 = execution_error(
            "Boost.Python library does not support enums with duplicate values. "
            "You can read more about this here: "
            "http://boost.org/libs/python/todo.html#support-for-enums-with-duplicate-values . "
            "The quick work around is to add new class variable to the exported enum, from Python. " )

W1033 = compilation_error( "`Py++` can not expose anonymous variables" )

W1034 = compilation_error( "`Py++` can not expose alignment bit." )

W1035 = compilation_error( "`Py++` can not expose static pointer member variables. This could be changed in future." )

W1036 = compilation_error( "`Py++` can not expose pointer to Python immutable member variables. This could be changed in future." )

W1037 = compilation_error(
            "Boost.Python library can not expose variables, which are pointer to function."
            " See http://www.boost.org/libs/python/doc/v2/faq.html#funcptr for more information." )

W1038 = compilation_error( "`Py++` can not expose variables of with anonymous type." )

W1039 = compilation_error( "`Py++` doesn't expose private or protected member variables." )

W1040 = execution_error(
            'The declaration is unexposed, but there are other declarations, which refer to it. '
            'This could cause "no to_python converter found" run time error. '
            'Declarations: %s' )

W1041 = warning(
            'Property "%s" could not be created. There is another exposed declaration with the same name( alias )." '
            'The property will make it inaccessible.' )

W1042 = warning(
            '`Py++` can not find out container value_type( mapped_type ). '
            'The container class is template instantiation declaration and not definition. '
            'This container class will be exported, but there is a possibility, that '
            'generated code will not compile or will lack some functionality. '
            'The solution to the problem is to create a variable of the class.' )

W1043 = warning( '`Py++` created an ugly alias ("%s") for template instantiated class.' )

W1044 = warning( '`Py++` created an ugly alias ("%s") for function wrapper.' )

W1045 = compilation_error(
            '`Py++` does not expose static arrays with unknown size. '
            'You can fix this by setting array size to the actual one.'
            'For more information see "array_t" class documentation.' )

W1046 = warning(
            'The virtual function was declared with empty throw. '
            'Adding the ability to override the function from Python breaks the exception specification. '
            'The function wrapper can throw any exception. '
            'In case of exception in run-time, the behaviour of the program is undefined! ' )

W1047 = warning(
            'There are two or more classes that use same alias("%s"). '
            'Duplicated aliases causes few problems, but the main one is that some '
            'of the classes will not be exposed to Python.'
            'Other classes : %s' )

W1048 = warning(
            'There are two or more aliases within "pyplusplus::aliases" namespace for '
            'the class. `Py++` selected "%s" as class alias. Other aliases: %s' )

W1049 = warning(
            'This method could not be overriden in Python - method returns reference '
            'to local variable!' )

W1050 = compilation_error(
            'The function returns "%s" type. You have to specify a call policies.'
            'Be sure to take a look on `Py++` defined call policies' )

W1051 = warning(
            'The function takes as argument (name=%s, pos=%d) "%s" type. '
            'You have to specify a call policies or to use "Function Transformation" '
            'functionality.' )

W1052 = warning(
            '`Py++` will not expose free operator "%s" - all classes, this operator works on, are excluded.' )

W1053 = warning(
            '`Py++` will not expose function "%s" - the function has variable-argument list, spicified by ellipsis (...).' )

W1054 = compilation_error( '`Py++` can not expose unions.' )

W1055 = warning( "`Py++` will generate class wrapper - hand written code should be added to the wrapper class destructor body" )

W1056 = compilation_error( "`Py++` can not expose array of pointers of Python immutable types. Take a look on 'ctypes integration' feature." )

W1057 = compilation_error( '`Py++` can not expose "%s" - it does not belong to named class.' )

W1058 = compilation_error( '`Py++` can not expose "%s" it belongs to anonymous class'
                           ' and requires additional code to expose.'
                           ' This could be changed in future.')

W1059 = compilation_error( '`Py++` can not expose "%s" - it requires additional code to expose.'
                           ' This could be changed in future.')

W1060 = compilation_error( '`Py++` can not expose "%s" - it has name, `Py++` only exposes anonymous unions.'
                           ' This could be changed in future.')

W1061 = compilation_error( '`Py++` can not expose "%s" - its type is "%s".'
                           ' This could be changed in future.')

W1062 = compilation_error( '"%s" contains "fake constructor" "%s", that was excluded.'
                           ' `Py++` will not generate "__init__" method, based on that function.')

W1063 = compilation_error( '"%s" contains "fake constructor" "%s", that is exportable.'
                           ' `Py++` will not generate "__init__" method, based on that function.')

W1064 = compilation_error( '`Py++` can not expose "%s" as "fake constructor" of "%s".'
                           ' Only the following function types supported: %s' )

W1065 = code_generation_error(
            'There are two or more classes that use same class wrapper alias("%s"). '
            'Duplicated class wrapper aliases causes few problems, but the main one is that during '
            'files generation `Py++` uses class wrapper aliases for the file names. '
            '`Py++` will rewrite the file content and at best will introduce compile time error. '
            'The worst case scenario: you will discover the problem during run-time.'
            'Use `wrapper_alias` property to change class wrapper alias value'
            'Other classes : %s' )

warnings = globals()

all_warning_msgs = []

for identifier, explanation in list(warnings.items()):
    if len( identifier ) != 5:
        continue
    if identifier[0] != 'W':
        continue
    try:
        int( identifier[1:] )
    except:
        continue
    msg = '%s %s: %s' % ( explanation.__class__.prefix, identifier, str(explanation) )
    msg_inst = explanation.__class__( msg, identifier )
    globals()[ identifier ] = msg_inst
    all_warning_msgs.append( msg_inst )


del warnings
del identifier
del explanation


if __name__ == '__main__':
    x = W1051 % ( 'xxxxxxxx', 122, 'yyyyyyyyyy' )
    print(x)
    print(x.__class__.__name__)

    print('\n\n\n')

    y = W1000
    print(y)

