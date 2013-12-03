//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
	   
#ifndef VSTDLIB_H
#define VSTDLIB_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------
#ifdef VSTDLIB_DLL_EXPORT
#define VSTDLIB_INTERFACE	DLL_EXPORT
#define VSTDLIB_OVERLOAD	DLL_GLOBAL_EXPORT
#define VSTDLIB_CLASS		DLL_CLASS_EXPORT
#define VSTDLIB_GLOBAL		DLL_GLOBAL_EXPORT
#else
#define VSTDLIB_INTERFACE	DLL_IMPORT
#define VSTDLIB_OVERLOAD	DLL_GLOBAL_IMPORT
#define VSTDLIB_CLASS		DLL_CLASS_IMPORT
#define VSTDLIB_GLOBAL		DLL_GLOBAL_IMPORT
#endif
 
#endif // VSTDLIB_H
