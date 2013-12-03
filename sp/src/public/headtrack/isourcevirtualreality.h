//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the IHeadTrack interface, which is implemented in headtrack.dll
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef ISOURCEVIRTUALREALITY_H
#define ISOURCEVIRTUALREALITY_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "tier1/refcount.h"
#include "appframework/IAppSystem.h"
#include "mathlib/vmatrix.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------
struct VRTrackerState_t
{
	// Tracker has finished starting up and has produced at least one valid pose.
	bool bInitialized;

	// Tracker currently has a valid pose.
	bool bHasValidPose;

	// Tracker is in a state where it is likely to suffer from yaw drift. This
	// would apply to any gyro-only tracking system.
	bool bWillDriftInYaw;
};


// NOTE NOTE NOTE!!!!  If you up this, grep for "NEW_INTERFACE" to see if there is anything
// waiting to be enabled during an interface revision.
#define SOURCE_VIRTUAL_REALITY_INTERFACE_VERSION "SourceVirtualReality001"

//-----------------------------------------------------------------------------
// The ISourceVirtualReality interface
//-----------------------------------------------------------------------------



abstract_class ISourceVirtualReality : public IAppSystem
{
public:
	virtual ~ISourceVirtualReality() {}

	// Placeholder for API revision
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	// This enum is used to tell some of the other calls in this interface which eye
	// is being requested.
	enum VREye
	{
		VREye_Left = 0,
		VREye_Right
	};

	// ----------------------------------------------------------------------
	// General utilities
	// ----------------------------------------------------------------------

	// Returns true if the game should run in VR mode
	virtual bool ShouldRunInVR() = 0;

	// The name of the display at which the game should put its window. 
	// TODO: This is pretty horrible from a "what the game has to do" point
	// of view. Make it better.
	virtual const char *GetDisplayName() = 0;

	// The size of the window that the game should create
	virtual bool GetWindowSize( int *pnWidth, int *pnHeight ) = 0;

	// Lets engine tell headtrack that it's going to use a different size window based
	// what the display is actually using. This happens when somebody clones
	// their desktop onto the HMD
	virtual void OverrideWindowSize( int nWidth, int nHeight ) = 0;

	// The size and position of the viewport for the specified eye
	virtual void GetViewportBounds( VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight ) = 0;

	// Performs the distortion post-processing.
	virtual bool DoDistortionProcessing ( const vrect_t *SrcRect ) = 0;

	// ----------------------------------------------------------------------
	// Getting the current pose
	// ----------------------------------------------------------------------

	// returns the pose relative to the zero point
	virtual VMatrix GetMideyePose() = 0;

	// returns the gravity-relative HUD correction matrix (I think)
	virtual VMatrix GetHudUpCorrection() = 0;

	// transforms to mid eye form left/right
	virtual VMatrix GetMidEyeFromLeft() = 0;
	virtual VMatrix GetMidEyeFromRight() = 0;

	// All-in-one interfaces (they call GetCameraPoseZeroFromCurrent)
	// Grabs the current tracking data and sets up state for the Override* calls.
	virtual bool SampleTrackingState ( float PlayerGameFov, float fPredictionSeconds ) = 0;

	// ----------------------------------------------------------------------
	// Information about the display
	// ----------------------------------------------------------------------

	// returns the serial number for the display or NULL if no serial number
	// could be retrieved
	virtual const char *GetDisplaySerialNumber() = 0;

	// returns the model number for the display or NULL if no model number
	// could be retrieved
	virtual const char *GetDisplayModelNumber() = 0;

	// returns the "ipd" of the display. This is the separation of the centers
	// of the two lenses in mm
	virtual float GetDisplaySeparationMM() = 0;

	// Computes and returns the projection matrix for the eye
	virtual bool GetEyeProjectionMatrix ( VMatrix *pResult, VREye, float zNear, float zFar, float fovScale ) = 0;

	// Returns the horizontal FOV of the display in degrees
	virtual float GetHorizontalFOVDegrees() = 0;

	// ----------------------------------------------------------------------
	// Information about the user
	// ----------------------------------------------------------------------

	// returns the intrapupilar distance of the user in mm
	virtual float GetUserIPDMM() = 0;

	// sets the intrapupilar distance of the user in mm
	virtual void SetUserIPDMM( float fIPDMM ) = 0;


	// ----------------------------------------------------------------------
	// Information about the tracker
	// ----------------------------------------------------------------------

	// returns the state of the tracking system
	virtual VRTrackerState_t GetTrackerState() = 0;
};



//-----------------------------------------------------------------------------

extern ISourceVirtualReality *g_pSourceVR;

inline bool UseVR()
{
	return g_pSourceVR != NULL && g_pSourceVR->ShouldRunInVR();
}

#endif // ISOURCEVIRTUALREALITY_H
