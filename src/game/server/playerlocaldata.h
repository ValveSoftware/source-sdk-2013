//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERLOCALDATA_H
#define PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif


#include "playernet_vars.h"
#include "networkvar.h"
#include "fogcontroller.h"

//-----------------------------------------------------------------------------
// Purpose: Player specific data ( sent only to local player, too )
//-----------------------------------------------------------------------------
class CPlayerLocalData
{
public:
	// Save/restore
	DECLARE_SIMPLE_DATADESC();
	// Prediction data copying
	DECLARE_CLASS_NOBASE( CPlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	CPlayerLocalData();

	void UpdateAreaBits( CBasePlayer *pl, unsigned char chAreaPortalBits[MAX_AREA_PORTAL_STATE_BYTES] );


public:

	CNetworkArray( unsigned char, m_chAreaBits, MAX_AREA_STATE_BYTES );								// Which areas are potentially visible to the client?
	CNetworkArray( unsigned char, m_chAreaPortalBits, MAX_AREA_PORTAL_STATE_BYTES );	// Which area portals are open?

	CNetworkVar( int,	m_iHideHUD );		// bitfields containing sections of the HUD to hide
	CNetworkVar( float, m_flFOVRate );		// rate at which the FOV changes (defaults to 0)
		
	Vector				m_vecOverViewpoint;			// Viewpoint overriding the real player's viewpoint
	
	// Fully ducked
	CNetworkVar( bool, m_bDucked );
	// In process of ducking
	CNetworkVar( bool, m_bDucking );
	// In process of duck-jumping
	CNetworkVar( bool, m_bInDuckJump );
	// During ducking process, amount of time before full duc
	CNetworkVar( float, m_flDucktime );
	CNetworkVar( float, m_flDuckJumpTime );
	// Jump time, time to auto unduck (since we auto crouch jump now).
	CNetworkVar( float, m_flJumpTime );
	// Step sound side flip/flip
	int m_nStepside;
	// Velocity at time when we hit ground
	CNetworkVar( float, m_flFallVelocity );
	// Previous button state
	int m_nOldButtons;
	float m_flOldForwardMove;
	class CSkyCamera *m_pOldSkyCamera;
	// Base velocity that was passed in to server physics so 
	//  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	// auto-decaying view angle adjustment
	CNetworkQAngle( m_vecPunchAngle );		
	CNetworkQAngle( m_vecPunchAngleVel );
	// Draw view model for the player
	CNetworkVar( bool, m_bDrawViewmodel );

	// Is the player wearing the HEV suit
	CNetworkVar( bool, m_bWearingSuit );
	CNetworkVar( bool, m_bPoisoned );
	CNetworkVar( bool, m_bForceLocalPlayerDraw );
	CNetworkVar( float, m_flStepSize );
	CNetworkVar( bool, m_bAllowAutoMovement );

	// 3d skybox
	CNetworkVarEmbedded( sky3dparams_t, m_skybox3d );
	// world fog
	CNetworkVarEmbedded( fogplayerparams_t, m_PlayerFog );
	fogparams_t			m_fog;
	// audio environment
	CNetworkVarEmbedded( audioparams_t, m_audio );

	CNetworkVar( bool, m_bSlowMovement );

	CNetworkString( m_szScriptOverlayMaterial, MAX_PATH );
};

EXTERN_SEND_TABLE(DT_Local);


#endif // PLAYERLOCALDATA_H
