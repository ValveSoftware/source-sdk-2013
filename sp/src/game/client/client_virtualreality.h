//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The implementation of ISourceVirtualReality, which provides utility
//			functions for VR including head tracking, window/viewport information,
//			rendering information, and distortion
//
//=============================================================================

#ifndef CLIENTVIRTUALREALITY_H
#define CLIENTVIRTUALREALITY_H
#if defined( _WIN32 )
#pragma once
#endif

#include "tier3/tier3.h"
#include "iclientvirtualreality.h"
#include "view_shared.h"

enum HeadtrackMovementMode_t
{
	HMM_SHOOTFACE_MOVEFACE = 0,		// Shoot from your face, move along your face.
	HMM_SHOOTFACE_MOVETORSO,		// Shoot from your face, move the direction your torso is facing.
	HMM_SHOOTMOUSE_MOVEFACE,		// Shoot from the mouse cursor which moves within the HUD, move along your face.
	HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEFACE,	// Shoot from the mouse cursor which moves, bounded within the HUD, move along your face.
	HMM_SHOOTBOUNDEDMOUSE_LOOKFACE_MOVEMOUSE,	// Shoot from the mouse cursor which moves, bounded within the HUD, move along your weapon (the "mouse")

	// The following are not intended to be user-selectable modes, they are used by e.g. followcam stuff.
	HMM_SHOOTMOVELOOKMOUSEFACE,		// Shoot & move & look along the mouse cursor (i.e. original unchanged gameplay), face just looks on top of that.
	HMM_SHOOTMOVEMOUSE_LOOKFACE,	// Shoot & move along the mouse cursor (i.e. original unchanged gameplay), face just looks.
	HMM_SHOOTMOVELOOKMOUSE,			// Shoot, move and look along the mouse cursor - HMD orientation is completely ignored!

	HMM_LAST,

	HMM_NOOVERRIDE = HMM_LAST		// Used as a retrun from ShouldOverrideHeadtrackControl(), not an actual mode.
};


//-----------------------------------------------------------------------------
// The implementation
//-----------------------------------------------------------------------------


class CClientVirtualReality: public CTier3AppSystem< IClientVirtualReality >
{
	typedef CTier3AppSystem< IClientVirtualReality > BaseClass;

public:

	CClientVirtualReality();
	~CClientVirtualReality();

	//---------------------------------------------------------
	// Initialization and shutdown
	//---------------------------------------------------------

	//
	// IAppSystem
	//
	virtual bool							Connect( CreateInterfaceFn factory );
	virtual void							Disconnect();
	virtual void *							QueryInterface( const char *pInterfaceName );

	// these will come from the engine
	virtual InitReturnVal_t					Init();
	virtual void							Shutdown();

	// Called when startup is complete
	void StartupComplete();

	//---------------------------------------------------------
	// IClientVirtualReality implementation
	//---------------------------------------------------------
	virtual void DrawMainMenu() OVERRIDE;


	//---------------------------------------------------------
	// VR utilities for use in the client
	//---------------------------------------------------------
	bool OverrideView ( CViewSetup *pViewMiddle, Vector *pViewModelOrigin, QAngle *pViewModelAngles, HeadtrackMovementMode_t hmmMovementOverride );
	bool OverrideStereoView( CViewSetup *pViewMiddle, CViewSetup *pViewLeft, CViewSetup *pViewRight );
	bool OverridePlayerMotion( float flInputSampleFrametime, const QAngle &oldAngles, const QAngle &curAngles, const Vector &curMotion, QAngle *pNewAngles, Vector *pNewMotion );
	bool OverrideWeaponHudAimVectors ( Vector *pAimOrigin, Vector *pAimDirection );
	bool CurrentlyZoomed();
	void OverrideTorsoTransform( const Vector & position, const QAngle & angles ) ;
	void CancelTorsoTransformOverride( ) ;
	bool CanOverlayHudQuad();
	void GetHUDBounds( Vector *pViewer, Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR );
	void RenderHUDQuad( bool bBlackout, bool bTranslucent );
	float GetZoomedModeMagnification();
	bool ProcessCurrentTrackingState( float fGameFOV );
	const VMatrix &GetHudProjectionFromWorld();
	void GetTorsoRelativeAim( Vector *pPosition, QAngle *pAngles );
	float GetHUDDistance();
	bool ShouldRenderHUDInWorld();
	const VMatrix & GetWorldFromMidEye() const { return m_WorldFromMidEyeNoDebugCam; }
	void OverrideViewModelTransform( Vector & vmorigin, QAngle & vmangles, bool bUseLargeOverride );
	void AlignTorsoAndViewToWeapon();
	void PostProcessFrame( StereoEye_t eEye );
	void OverlayHUDQuadWithUndistort( const CViewSetup &view, bool bDoUndistort, bool bBlackout, bool bTranslucent );

	//---------------------------------------------------------
	// Enter/leave VR mode
	//---------------------------------------------------------
	void Activate();
	void Deactivate();

private:
	HeadtrackMovementMode_t m_hmmMovementActual;

	// Where the current mideye is relative to the (game)world.
	VMatrix			m_WorldFromMidEye;

	// used for drawing the HUD
	float			m_fHudHorizontalFov;
	VMatrix			m_WorldFromHud;
	VMatrix			m_HudProjectionFromWorld;
	float			m_fHudHalfWidth;
	float			m_fHudHalfHeight;

	// Where the current mideye is relative to the zero (torso) (currently always the same as m_MideyeZeroFromMideyeCurrent!)
	VMatrix			m_TorsoFromMideye;

	// The debug cam will play with the above, but some things want the non-debug view.
	VMatrix			m_WorldFromMidEyeNoDebugCam;

	// Where the weapon is currently pointing (note the translation will be zero - this is just orientation)
	VMatrix			m_WorldFromWeapon;

	// The player's current torso angles/pos in the world.
	QAngle			m_PlayerTorsoAngle;
	Vector			m_PlayerTorsoOrigin;
	Vector			m_PlayerLastMovement;

	// The player's current view angles/pos in the world.
	QAngle			m_PlayerViewAngle;
	Vector			m_PlayerViewOrigin;

	// The amount of zoom to apply to the view of the world (but NOT to the HUD!). Used for sniper weapons, etc.
	float			m_WorldZoomScale;

	// for overriding torso position in vehicles
	QAngle			m_OverrideTorsoAngle;
	QAngle			m_OverrideTorsoOffset;
	bool			m_bOverrideTorsoAngle;

	// While this is >0, we keep forcing the torso (and maybe view) to the weapon.
	int				m_iAlignTorsoAndViewToWeaponCountdown;

	bool			m_bMotionUpdated;

	RTime32			m_rtLastMotionSample;

	// video mode we had before we entered VR mode
	bool m_bNonVRWindowed;
	int m_nNonVRWidth;
	int m_nNonVRHeight;
#if defined( USE_SDL )
    int m_nNonVRSDLDisplayIndex;
#endif
    bool m_bNonVRRawInput;
};

extern CClientVirtualReality g_ClientVirtualReality;

#endif // CLIENTVIRTUALREALITY_H
