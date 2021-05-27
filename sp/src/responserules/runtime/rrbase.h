//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RRBASE_H
#define RRBASE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef _WIN32
// Silence certain warnings
// #pragma warning(disable : 4244)		// int or float down-conversion
// #pragma warning(disable : 4305)		// int or float data truncation
// #pragma warning(disable : 4201)		// nameless struct/union
// #pragma warning(disable : 4511)     // copy constructor could not be generated
// #pragma warning(disable : 4675)     // resolved overload was found by argument dependent lookup
#endif

#ifdef _DEBUG
#define DEBUG 1
#endif

// Misc C-runtime library headers
#include <math.h>
#include <ctype.h>
#include <stdio.h>

// tier 0
#include "tier0/dbg.h"
#include "tier0/platform.h"
#include "basetypes.h"

// tier 1
#include "tier1/strtools.h"
#include "utlvector.h"
#include "utlsymbol.h"

// tier 2
#include "string_t.h"

// Shared engine/DLL constants
#include "const.h"
#include "edict.h"

// app
#if defined(_X360)
#define DISABLE_DEBUG_HISTORY 1
#endif

#include "responserules/response_types.h"
#include "response_types_internal.h"
#include "responserules/response_host_interface.h"


#endif // CBASE_H
