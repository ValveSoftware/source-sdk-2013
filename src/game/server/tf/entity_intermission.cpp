//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Intermission.
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_intermission.h"



LINK_ENTITY_TO_CLASS( point_intermission, CTFIntermission );

BEGIN_DATADESC(CTFIntermission)
	// Keys

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Activate", InputActivate ),

	// Outputs

END_DATADESC()

//=============================================================================
//
// CTF Intermission functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CTFIntermission::InputActivate( inputdata_t &inputdata )
{
	CTFGameRules *pCTFGameRules = TFGameRules();
	if ( pCTFGameRules )
	{
		pCTFGameRules->GoToIntermission();
	}
}


