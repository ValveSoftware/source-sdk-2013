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
class ITexture;
class IMaterialSystem;

//-----------------------------------------------------------------------------
// important enumeration
//-----------------------------------------------------------------------------

struct VRRect_t
{
	int32 nX;
	int32 nY;
	int32 nWidth;
	int32 nHeight;
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

	// Which texture is being requested in GetRenderTarget?
	enum EWhichRenderTarget
	{
		RT_Color = 0,
		RT_Depth,
	};


	// ----------------------------------------------------------------------
	// General utilities
	// ----------------------------------------------------------------------

	// Returns true if the game should run in VR mode
	virtual bool ShouldRunInVR() = 0;

	// Returns true if there is a compatible HMD connected 
	virtual bool IsHmdConnected() = 0;

	// The size and position of the viewport for the specified eye
	virtual void GetViewportBounds( VREye eEye, int *pnX, int *pnY, int *pnWidth, int *pnHeight ) = 0;

	// Performs the distortion post-processing.
	virtual bool DoDistortionProcessing ( VREye eEye ) = 0;

	// Composites the HUD directly onto the backbuffer / render target, including undistort.
	virtual bool CompositeHud ( VREye eEye, float ndcHudBounds[4], bool bDoUndistort, bool bBlackout, bool bTranslucent ) = 0;

	// ----------------------------------------------------------------------
	// Getting the current pose
	// ----------------------------------------------------------------------

	// returns the pose relative to the zero point
	virtual VMatrix GetMideyePose() = 0;

	// All-in-one interfaces (they call GetCameraPoseZeroFromCurrent)
	// Grabs the current tracking data and sets up state for the Override* calls.
	virtual bool SampleTrackingState ( float PlayerGameFov, float fPredictionSeconds ) = 0;

	// ----------------------------------------------------------------------
	// Information about the display
	// ----------------------------------------------------------------------

	// Passes back the bounds of the window that the game should create. This might
	// span two displays if we're dealing with a two-input display. Returns true
	// if the bounds were set.
	virtual bool GetDisplayBounds( VRRect_t *pRect ) = 0;

	// Computes and returns the projection matrix for the eye
	virtual bool GetEyeProjectionMatrix ( VMatrix *pResult, VREye, float zNear, float zFar, float fovScale ) = 0;

	// Returns the transform from the mid-eye to the specified eye. Multiply this by 
	// the tweaked (for mouse rotation and WASD translation) mideye position to get the
	// view matrix. This matrix takes the user's IPD into account.
	virtual VMatrix GetMidEyeFromEye( VREye eEye ) = 0;

	// returns the adapter index to use for VR mode
	virtual int GetVRModeAdapter() = 0;

	// ----------------------------------------------------------------------
	// Information about the tracker
	// ----------------------------------------------------------------------

	virtual bool WillDriftInYaw() = 0;

	// ----------------------------------------------------------------------
	// Methods about oversized offscreen rendering
	// ----------------------------------------------------------------------

	// Sets up the pre-distortion render targets.
	virtual void CreateRenderTargets( IMaterialSystem *pMaterialSystem ) = 0;
	virtual void ShutdownRenderTargets() = 0;

	// fetches the render target for the specified eye
	virtual ITexture *GetRenderTarget( VREye eEye, EWhichRenderTarget eWhich ) = 0;

	// Returns the (possibly overridden) framebuffer size for render target sizing.
	virtual void				GetRenderTargetFrameBufferDimensions( int & nWidth, int & nHeight ) = 0;

	// ----------------------------------------------------------------------
	// Enter/leave VR mode
	// ----------------------------------------------------------------------
	virtual bool Activate() = 0;
	virtual void Deactivate() = 0;
	
	virtual bool ShouldForceVRMode() = 0;
	virtual void SetShouldForceVRMode() = 0;

};



//-----------------------------------------------------------------------------

extern ISourceVirtualReality *g_pSourceVR;

inline bool UseVR()
{
	return g_pSourceVR != NULL && g_pSourceVR->ShouldRunInVR();
}

inline bool ShouldForceVRActive()
{
	return g_pSourceVR != NULL && g_pSourceVR->ShouldForceVRMode();
}

#endif // ISOURCEVIRTUALREALITY_H
