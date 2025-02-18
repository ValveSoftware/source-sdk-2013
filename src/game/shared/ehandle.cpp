//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module implements functions to support ehandles.
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( GAME_DLL )
	
	#include "entitylist.h"


	void DebugCheckEHandleAccess( void *pEnt )
	{
		extern bool g_bDisableEhandleAccess;

		if ( g_bDisableEhandleAccess )
		{
			Msg( "Access of EHANDLE/CHandle for class %s:%p in destructor!\n",
				STRING(((CBaseEntity*)pEnt)->m_iClassname ), pEnt );
		}
	}

#else
	
#endif


