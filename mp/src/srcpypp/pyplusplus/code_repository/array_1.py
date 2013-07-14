# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This file contains C++ code needed to export one dimensional static arrays.
"""


namespace = "pyplusplus::containers::static_sized"

file_name = "__array_1.pypp.hpp"

code = \
"""// Copyright 2004-2008 Roman Yakovenko.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef __array_1_pyplusplus_hpp__
#define __array_1_pyplusplus_hpp__

#include "boost/python.hpp"
#include "boost/mpl/if.hpp"
#include "boost/type_traits/is_same.hpp"
#include "boost/type_traits/is_fundamental.hpp"
#include "boost/python/converter/registry.hpp"

#include <iostream>

//1 - dimension
namespace pyplusplus{ namespace containers{ namespace static_sized{

inline void
raise_on_out_of_range( long unsigned int size, long unsigned int index ){
    if( size <= index ){
        throw std::out_of_range("index out of range");
    }
}

namespace details{

template<class T>
struct is_immutable{
    BOOST_STATIC_CONSTANT( 
        bool
        , value = ( boost::is_same< T, std::string >::value )
                  || ( boost::is_same< T, std::wstring >::value )
                  || ( boost::is_fundamental< T >::value )
                  || ( boost::is_enum< T >::value )
    );

};

template<class T>
bool is_registered(){
    namespace bpl = boost::python;
    bpl::handle<> class_obj( bpl::objects::registered_class_object( bpl::type_id< T >()));
    return class_obj.get() ? true : false;
}

template< class T >
void register_alias( const char* name ){
    namespace bpl = boost::python;
    bpl::handle<> class_obj( bpl::objects::registered_class_object( bpl::type_id< T >()));
    boost::python::scope().attr( name ) = bpl::object( class_obj );
}

}//details

template< class TItemType, long unsigned int size >
struct const_array_1_t{
    
    typedef BOOST_DEDUCED_TYPENAME boost::call_traits<const TItemType>::param_type param_type;
    
    typedef BOOST_DEDUCED_TYPENAME boost::mpl::if_c< 
            details::is_immutable<TItemType>::value
            , TItemType
            , param_type
        >::type reference_type;

    const_array_1_t( TItemType const * const data )
    : m_data( data ){
        if( !data ){
            throw std::runtime_error( "const_array_1_t: pointer to null has been recieved." );
        }
    }

    long unsigned int len() const {
        return size;
    }

    reference_type item_ref( long unsigned int index ) const{
        raise_on_out_of_range( size, index );
        return m_data[index];
    }

private:

    TItemType const * m_data;

};

template< class TItemType, long unsigned int size >
struct array_1_t{

    typedef BOOST_DEDUCED_TYPENAME boost::call_traits<const TItemType>::param_type param_type;
    
    typedef BOOST_DEDUCED_TYPENAME boost::mpl::if_c< 
            details::is_immutable<TItemType>::value
            , TItemType
            , param_type
        >::type reference_type;

    array_1_t( TItemType * data )
    : m_data( data ){
        if( !data ){
            throw std::runtime_error( "array_1_t: pointer to null has been recieved." );
        }
    }

    long unsigned int len() const {
        return size;
    }

    reference_type item_ref( long unsigned int index ) const{
        raise_on_out_of_range( size, index );
        return m_data[index];
    }

    void
    set_item( long unsigned int index, reference_type new_value ){
        raise_on_out_of_range( size, index );
        m_data[index] = new_value;
    }

private:

    TItemType* m_data;

};

template< class TItemType
          , long unsigned int size
          , typename CallPolicies=boost::python::default_call_policies >
struct register_const_array_1{
    register_const_array_1(const char* name){
        namespace bpl = boost::python;
        typedef const_array_1_t< TItemType, size > wrapper_t;

        if( details::is_registered< wrapper_t >() ){
            details::register_alias< wrapper_t >( name );
        }
        else{
            bpl::class_< wrapper_t >( name, bpl::no_init )
                .def( "__getitem__"
                      , &wrapper_t::item_ref
                      , ( bpl::arg("index") )
                      , CallPolicies() )
                .def( "__len__", &wrapper_t::len );
        }
    }
};

template< class TItemType
          , long unsigned int size
          , typename CallPolicies=boost::python::default_call_policies >
struct register_array_1{
    register_array_1(const char* name){
        namespace bpl = boost::python;
        typedef array_1_t< TItemType, size > wrapper_t;
        if( details::is_registered< wrapper_t >() ){
            details::register_alias< wrapper_t >( name );
        }
        else{
            bpl::class_< wrapper_t >( name, bpl::no_init )
                .def( "__getitem__"
                      , &wrapper_t::item_ref
                      , ( bpl::arg("index") )
                      , CallPolicies() )
                .def( "__setitem__"
                      , &wrapper_t::set_item
                      , ( bpl::arg("index"), bpl::arg("value") )
                      , CallPolicies()  )
                .def( "__len__", &wrapper_t::len );
        }
    }
};

} /*pyplusplus*/ } /*containers*/ } /*static_sized*/


#endif//__array_1_pyplusplus_hpp__

"""
