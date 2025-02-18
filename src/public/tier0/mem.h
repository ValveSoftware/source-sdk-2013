//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Memory allocation!
//
// $NoKeywords: $
//=============================================================================//

#ifndef TIER0_MEM_H
#define TIER0_MEM_H

#ifdef _WIN32
#pragma once
#endif

#include <stddef.h>

#include "tier0/platform.h"

#if !defined(STATIC_TIER0) && !defined(_STATIC_LINKED)

#ifdef TIER0_DLL_EXPORT
#  define MEM_INTERFACE DLL_EXPORT
#else
#  define MEM_INTERFACE DLL_IMPORT
#endif

#else // BUILD_AS_DLL

#define MEM_INTERFACE extern

#endif // BUILD_AS_DLL



//-----------------------------------------------------------------------------
// DLL-exported methods for particular kinds of memory
//-----------------------------------------------------------------------------
MEM_INTERFACE void *MemAllocScratch( int nMemSize );
MEM_INTERFACE void MemFreeScratch();
MEM_INTERFACE void MemAllocOOMError( size_t nSize );

#if defined(_LINUX) && !defined( USE_DXVK )
MEM_INTERFACE void ZeroMemory( void *mem, size_t length );
#endif


#endif /* TIER0_MEM_H */
