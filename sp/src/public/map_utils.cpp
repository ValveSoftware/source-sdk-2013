//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "mathlib/vector.h"
#include "bspfile.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SetupLightNormalFromProps( const QAngle &angles, float angle, float pitch, Vector &output )
{
	if (angle == ANGLE_UP)
	{
		output[0] = output[1] = 0;
		output[2] = 1;
	}
	else if (angle == ANGLE_DOWN)
	{
		output[0] = output[1] = 0;
		output[2] = -1;
	}
	else
	{
		// if we don't have a specific "angle" use the "angles" YAW
		if ( !angle )
		{
			angle = angles[YAW];
		}
		
		output[2] = 0;
		output[0] = (float)cos (angle/180*M_PI);
		output[1] = (float)sin (angle/180*M_PI);
	}
	
	if ( !pitch )
	{
		// if we don't have a specific "pitch" use the "angles" PITCH
		pitch = angles[PITCH];
	}
	
	output[2] = (float)sin(pitch/180*M_PI);
	output[0] *= (float)cos(pitch/180*M_PI);
	output[1] *= (float)cos(pitch/180*M_PI);
}

