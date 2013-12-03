//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "rangecheckedvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_bDoRangeChecks = true;


static int g_nDisables = 0;


CDisableRangeChecks::CDisableRangeChecks()
{
	if ( !ThreadInMainThread() )
		return;
	g_nDisables++;
	g_bDoRangeChecks = false;
}


CDisableRangeChecks::~CDisableRangeChecks()
{
	if ( !ThreadInMainThread() )
		return;
	Assert( g_nDisables > 0 );
	--g_nDisables;
	if ( g_nDisables == 0 )
	{
		g_bDoRangeChecks = true;
	}
}




