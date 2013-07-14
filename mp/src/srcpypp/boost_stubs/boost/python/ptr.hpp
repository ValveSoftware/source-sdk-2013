#ifndef PTR_DWA20020601_HPP
# define PTR_DWA20020601_HPP

# if _MSC_VER+0 >= 1020
#  pragma once
# endif

namespace boost { namespace python {

template<class Ptr> class pointer_wrapper
{ 
 public:
    typedef Ptr type;
    
    explicit pointer_wrapper(Ptr x): p_(x) {}
    operator Ptr() const { return p_; }
    Ptr get() const { return p_; }
 private:
    Ptr p_;
};

template<class T>
inline pointer_wrapper<T> ptr(T t)
{ 
    return pointer_wrapper<T>(t);
}

# ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

template<typename T>
class unwrap_pointer
{
 public:
    typedef T type;
};

template<typename T>
class unwrap_pointer<pointer_wrapper<T> >
{
 public:
    typedef T type;
};
# else // no partial specialization

}} // namespace boost::python

#include <boost/type.hpp>

namespace boost { namespace python {

namespace detail
{
  typedef char (&yes_pointer_wrapper_t)[1];
  typedef char (&no_pointer_wrapper_t)[2];
      
  no_pointer_wrapper_t is_pointer_wrapper_test(...);

  template<typename T>
  yes_pointer_wrapper_t is_pointer_wrapper_test(boost::type< pointer_wrapper<T> >);

  template<bool wrapped>
  struct pointer_unwrapper
  {
      template <class T>
      struct apply
      {
          typedef T type;
      };
  };

  template<>
  struct pointer_unwrapper<true>
  {
      template <class T>
      struct apply
      {
          typedef typename T::type type;
      };
  };
}

template<typename T>
class is_pointer_wrapper
{
 public:
    BOOST_STATIC_CONSTANT(
        bool, value = (
        sizeof(detail::is_pointer_wrapper_test(boost::type<T>()))
            == sizeof(detail::yes_pointer_wrapper_t)));
    typedef mpl::bool_<value> type;
};

template <typename T>
class unwrap_pointer
    : public detail::pointer_unwrapper<
        is_pointer_wrapper<T>::value
      >::template apply<T>
{};

# endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

}} // namespace boost::python

#endif // #ifndef PTR_DWA20020601_HPP
