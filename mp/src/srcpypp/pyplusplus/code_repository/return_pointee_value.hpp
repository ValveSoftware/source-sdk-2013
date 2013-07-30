// Copyright 2004-2008 Roman Yakovenko.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RETURN_POINTEE_VALUE_9_11_2006
#define RETURN_POINTEE_VALUE_9_11_2006

#include <boost/python.hpp>

namespace boost{ namespace python{
    
namespace detail{
    
    struct make_value_holder
    {
        template <class T>
        static PyObject* execute(T* p)
        {
            if (p == 0)
            {
                return python::detail::none();
            }
            else
            {
                object p_value( *p );
                return incref( p_value.ptr() );
            }
        }
    };

    template <class R>
    struct return_pointee_value_requires_a_pointer_return_type
    # if defined(__GNUC__) && __GNUC__ >= 3 || defined(__EDG__)
    {}
    # endif
    ;

} //detail


struct return_pointee_value
{
    template <class T>
    struct apply
    {
        BOOST_STATIC_CONSTANT( bool, ok = is_pointer<T>::value );
        
        typedef typename mpl::if_c<
            ok
            , to_python_indirect<T, detail::make_value_holder>
            , detail::return_pointee_value_requires_a_pointer_return_type<T>
        >::type type;
    };
};


} } //boost::python 

#endif//RETURN_POINTEE_VALUE_9_11_2006
