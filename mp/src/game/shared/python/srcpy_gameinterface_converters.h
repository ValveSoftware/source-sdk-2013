//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef SRCPY_GAMEINTERFACE_CONVERTERS_H
#define SRCPY_GAMEINTERFACE_CONVERTERS_H
#ifdef _WIN32
#pragma once
#endif

class CTempEnts;
struct model_t;

#include "srcpy_gameinterface.h"

// Converters
struct ptr_model_t_to_wrap_model_t : boost::python::to_python_converter<model_t *, ptr_model_t_to_wrap_model_t>
{
	static PyObject* convert(model_t *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(wrap_model_t(s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct const_ptr_model_t_to_wrap_model_t : boost::python::to_python_converter<const model_t *, const_ptr_model_t_to_wrap_model_t>
{
	static PyObject* convert(const model_t *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(wrap_model_t((model_t *)s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct wrap_model_t_to_model_t
{
	wrap_model_t_to_model_t()
	{
		boost::python::converter::registry::insert(
			&extract_model_t, 
			boost::python::type_id<model_t>()
			);
	}

	static void* extract_model_t(PyObject* op) {
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
			boost::python::borrowed(op)
			)
		);
		wrap_model_t w = boost::python::extract<wrap_model_t>(handle);
		return w.pModel;
	}
};

#endif // SRCPY_GAMEINTERFACE_CONVERTERS_H