//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include <stdio.h>
#include "physdll.h"
#include "filesystem_tools.h"

static CSysModule *pPhysicsModule = NULL;
CreateInterfaceFn GetPhysicsFactory( void )
{
	if ( !pPhysicsModule )
	{
		pPhysicsModule = g_pFullFileSystem->LoadModule( "VPHYSICS.DLL" );
		if ( !pPhysicsModule )
			return NULL;
	}

	return Sys_GetFactory( pPhysicsModule );
}

void PhysicsDLLPath( const char *pPathname )
{
	if ( !pPhysicsModule )
	{
		pPhysicsModule = g_pFullFileSystem->LoadModule( pPathname );
	}
}
