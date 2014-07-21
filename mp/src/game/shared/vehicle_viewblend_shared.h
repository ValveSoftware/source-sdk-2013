//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef VEHICLE_VIEWBLEND_SHARED_H
#define VEHICLE_VIEWBLEND_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Definition for how to calculate a point on the remap curve
enum RemapAngleRange_CurvePart_t
{
	RemapAngleRange_CurvePart_Zero = 0,
	RemapAngleRange_CurvePart_Spline,
	RemapAngleRange_CurvePart_Linear,
};

// If we enter the linear part of the remap for curve for any degree of freedom, we can lock
// that DOF (stop remapping). This is useful for making flips feel less spastic as we oscillate
// randomly between different parts of the remapping curve.
struct ViewLockData_t
{
	float	flLockInterval;			// The duration to lock the view when we lock it for this degree of freedom.
	// 0 = never lock this degree of freedom.

	bool	bLocked;				// True if this DOF was locked because of the above condition.

	float	flUnlockTime;			// If this DOF is locked, the time when we will unlock it.

	float	flUnlockBlendInterval;	// If this DOF is locked, how long to spend blending out of the locked view when we unlock.
};

// This is separate from the base vehicle implementation so that any class 
// that derives from IClientVehicle can use it. To use it, contain one of the
// following structs, fill out the first section, and then call VehicleViewSmoothing()
// inside your GetVehicleViewPosition() function.
struct ViewSmoothingData_t
{
	DECLARE_SIMPLE_DATADESC();

	// Fill these out in your vehicle
	CBaseAnimating	*pVehicle;
	bool	bClampEyeAngles;	// Perform eye Z clamping
	float	flPitchCurveZero;	// Pitch values below this are clamped to zero.
	float	flPitchCurveLinear;	// Pitch values above this are mapped directly.
	//		Spline in between.
	float	flRollCurveZero;	// Pitch values below this are clamped to zero.
	float	flRollCurveLinear;	// Roll values above this are mapped directly.
	//		Spline in between.
	float	flFOV;				// FOV when in the vehicle.

	ViewLockData_t pitchLockData;
	ViewLockData_t rollLockData;

	bool	bDampenEyePosition;	// Only set to true for C_PropVehicleDriveable derived vehicles

	// Don't change these, they're used by VehicleViewSmoothing()
	bool	bRunningEnterExit;
	bool	bWasRunningAnim;
	float	flEnterExitStartTime;	// Time we began our animation at
	float	flEnterExitDuration;	// Duration of the animation
	QAngle	vecAnglesSaved;
	Vector	vecOriginSaved;
	QAngle	vecAngleDiffSaved;	// The original angular error between the entry/exit anim and player's view when we started playing the anim.
	QAngle	vecAngleDiffMin;	// Tracks the minimum angular error achieved so we can converge on the anim's angles.
};

// TEMP: Shared vehicle view smoothing
void SharedVehicleViewSmoothing(CBasePlayer *pPlayer, 
								Vector *pAbsOrigin, QAngle *pAbsAngles, 
								bool bEnterAnimOn, bool bExitAnimOn, 
								const Vector &vecEyeExitEndpoint, 
								ViewSmoothingData_t *pData, 
								float *pFOV );

#endif // VEHICLE_VIEWBLEND_SHARED_H
