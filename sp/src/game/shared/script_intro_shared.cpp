//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

//-----------------------------------------------------------------------------
// Purpose: Calculate the FOV for the intro sequence (needed by both server and client)
//-----------------------------------------------------------------------------
float ScriptInfo_CalculateFOV( float flFOVBlendStartTime, float flNextFOVBlendTime, int nFOV, int nNextFOV, bool bSplineRamp )
{
	// Handle the spline case
	if ( bSplineRamp )
	{
		//If we're past the zoom time, just take the new value and stop transitioning
		float deltaTime = (float)( gpGlobals->curtime - flFOVBlendStartTime ) / ( flNextFOVBlendTime - flFOVBlendStartTime );
		if ( deltaTime >= 1.0f )
			return nNextFOV;

		float flResult = SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, (float) nFOV, (float) nNextFOV );
		
		// Msg("FOV BLENDING: curtime %.2f    StartedAt %.2f    FinishAt: %.2f\n", gpGlobals->curtime, flFOVBlendStartTime, flNextFOVBlendTime );
		// Msg("			   Perc:   %.2f    Start: %d	End: %d		FOV: %.2f\n", SimpleSplineRemapVal( deltaTime, 0.0f, 1.0f, nFOV, nNextFOV ), nFOV, nNextFOV, flResult );
		
		return flResult;
	}
	
	// Common, linear blend
	if ( (flNextFOVBlendTime - flFOVBlendStartTime) != 0 )
	{
		float flResult = RemapValClamped( gpGlobals->curtime, flFOVBlendStartTime, flNextFOVBlendTime, (float) nFOV, (float) nNextFOV );
		
		// Msg("FOV BLENDING: curtime %.2f    StartedAt %.2f    FinishAt: %.2f\n", gpGlobals->curtime, flFOVBlendStartTime, flNextFOVBlendTime );
		// Msg("			   Perc:   %.2f    Start: %d	End: %d		FOV: %.2f\n", RemapValClamped( gpGlobals->curtime, flFOVBlendStartTime, flNextFOVBlendTime, 0.0, 1.0 ), nFOV, nNextFOV, flResult );
		
		return flResult;
	}

	
	// Msg("FOV BLENDING: JUMPED TO NEXT FOV (%d)\n", nNextFOV );
	return nNextFOV;
}
