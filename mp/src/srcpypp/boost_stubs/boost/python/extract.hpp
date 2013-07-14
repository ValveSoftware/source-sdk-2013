// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef EXTRACT_DWA200265_HPP
# define EXTRACT_DWA200265_HPP

# include <boost/python/object_core.hpp>

namespace boost { namespace python {

template <class T>
struct extract
{
public:
	extract(object const&);
};


//
// Implementations
//

template <class T>
inline extract<T>::extract(object const& o)
{
}

}} // namespace boost::python::converter

#endif // EXTRACT_DWA200265_HPP
