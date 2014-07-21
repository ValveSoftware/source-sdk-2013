//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef VEHICLE_CHOREO_GENERIC_SHARED_H
#define VEHICLE_CHOREO_GENERIC_SHARED_H

#if defined( _WIN32 )
#pragma once
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct vehicleview_t
{
	DECLARE_CLASS_NOBASE( vehicleview_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

	CNetworkVar( bool,	bClampEyeAngles );	// Perform eye Z clamping

	CNetworkVar( float,	flPitchCurveZero );	// Pitch values below this are clamped to zero.
	CNetworkVar( float,	flPitchCurveLinear );	// Pitch values above this are mapped directly.
	//		Spline in between.
	CNetworkVar( float,	flRollCurveZero );	// Pitch values below this are clamped to zero.
	CNetworkVar( float,	flRollCurveLinear );	// Roll values above this are mapped directly.
	//		Spline in between.
	CNetworkVar( float,	flFOV );				// FOV when in the vehicle.

	CNetworkVar( float, flYawMin );
	CNetworkVar( float, flYawMax );

	CNetworkVar( float, flPitchMin );
	CNetworkVar( float, flPitchMax );
};


#endif // VEHICLE_CHOREO_GENERIC_SHARED_H
