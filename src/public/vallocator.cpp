//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#if defined( OSX )
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include "vallocator.h"
#include "basetypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

VStdAllocator g_StdAllocator;

void* VStdAllocator::Alloc(unsigned long size)
{
	if(size)
	{
		void *ret = malloc(size);
		return ret;
	}
	else
		return 0;
}

void VStdAllocator::Free(void *ptr)
{
	free(ptr);
}

#endif // !_STATIC_LINKED || _SHARED_LIB
