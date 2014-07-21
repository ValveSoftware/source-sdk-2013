//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include "crtmemdebug.h"
#ifdef USECRTMEMDEBUG
#include <crtdbg.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CheckHeap( void )
{
#ifdef USECRTMEMDEBUG
	_CrtCheckMemory();
#endif
}

// undone: this needs to be somehow made to construct before everything else.
// see http://msdn.microsoft.com/library/periodic/period97/dembugs.htm for info
void InitCRTMemDebug( void )
{
#ifdef USECRTMEMDEBUG
	_CrtSetDbgFlag( 
//	_CRTDBG_ALLOC_MEM_DF  | 
	_CRTDBG_CHECK_ALWAYS_DF |
	_CRTDBG_CHECK_CRT_DF | 
	_CRTDBG_DELAY_FREE_MEM_DF );
#endif
}

#endif // !_STATIC_LINKED || _SHARED_LIB
