// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OBJECT_CORE_DWA2002615_HPP
# define OBJECT_CORE_DWA2002615_HPP

# include <boost/python/detail/wrap_python.hpp>

namespace boost { namespace python { 

	class object 
	{
	public:
		object();
		object( const object &a );
		// function call
		//
		object operator()() const;
		object operator()(object const &args) const; 
		object operator()(object const &args, object const &args2) const; 

		inline object& operator=(object const& rhs);

		object attr(char const*) const;
		// Underlying object access -- returns a borrowed reference
		//PyObject* ptr() const;
		PyObject *ptr() const;
	};

}} // namespace boost::python

#endif // OBJECT_CORE_DWA2002615_HPP
