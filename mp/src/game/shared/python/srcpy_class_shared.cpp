//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_class_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

boost::python::object CreatePyHandleHelper( const CBaseEntity *pEnt, const char *handlename )
{	
	boost::python::object clshandle;
	if( pEnt->GetPyInstance().ptr() != Py_None )	
	{							
		try {																
			clshandle = _entities.attr("PyHandle");							
		} catch(boost::python::error_already_set &) {					
			Warning("Failed to create a PyHandle\n");				
			PyErr_Print();																		
			PyErr_Clear();																		
			return boost::python::object();														
		}																						
		return clshandle(pEnt->GetPyInstance());														
	}																							
	try {																												
		clshandle = _entities.attr(handlename);													
	} catch(boost::python::error_already_set &) {												
		Warning("Failed to create handle %s\n", handlename);								
		PyErr_Print();																			
		PyErr_Clear();																			
		return boost::python::object();															
	}																							
	return clshandle(boost::python::ptr(pEnt));													
}
 