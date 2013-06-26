//===== Copyright (c), Valve Corporation, All rights reserved. ======//
//
// Purpose: Contains the IHeadTrack interface, which is implemented in headtrack.dll
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef IHEADTRACK_H
#define IHEADTRACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "tier1/refcount.h"
#include "appframework/IAppSystem.h"
#include "mathlib/vmatrix.h"
#include "view_shared.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------

// NOTE NOTE NOTE!!!!  If you up this, grep for "NEW_INTERFACE" to see if there is anything
// waiting to be enabled during an interface revision.
#define HEAD_TRACK_INTERFACE_VERSION "VHeadTrack001"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

abstract_class IHeadTrack : public IAppSystem
{
public:
	virtual ~IHeadTrack() {}

	// Placeholder for API revision
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;

	//-----------------------------------------------------------------------------
	// Tracking section
	//-----------------------------------------------------------------------------
    // All methods return true on success, false on failure.

    // Also sets the zero pose
    virtual bool ResetTracking() = 0;

    // Set the current pose as the "zero"
    virtual bool SetCurrentCameraAsZero() = 0;

	// Raw interfaces - one one or other of these, not both (GetCameraPoseZeroFromCurrent calls GetCameraFromWorldPose );
	virtual bool GetCameraFromWorldPose ( VMatrix *pResultCameraFromWorldPose, VMatrix *pResultCameraFromWorldPoseUnpredicted = NULL, double *pflAcquireTime = NULL ) = 0;
	virtual bool GetCameraPoseZeroFromCurrent ( VMatrix *pResultMatrix ) = 0;

	// All-in-one interfaces (they call GetCameraPoseZeroFromCurrent)
	// Grabs the current tracking data and sets up state for the Override* calls.
	virtual bool ProcessCurrentTrackingState ( float PlayerGameFov ) = 0;

	// Performs the distortion post-processing.
	virtual bool DoDistortionProcessing ( const vrect_t *SrcRect ) = 0;

};



//-----------------------------------------------------------------------------

extern IHeadTrack *g_pHeadTrack;

inline bool UseHeadTracking()
{
	return g_pHeadTrack != NULL;
}
#endif // IHEADTRACK_H
