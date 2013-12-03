//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines the player specific data that is sent only to the player
//			to whom it belongs.
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_PLAYERLOCALDATA_H
#define C_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "mathlib/vector.h"
#include "playernet_vars.h"

//-----------------------------------------------------------------------------
// Purpose: Player specific data ( sent only to local player, too )
//-----------------------------------------------------------------------------
class CPlayerLocalData
{
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( CPlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	CPlayerLocalData() :
		m_iv_vecPunchAngle( "CPlayerLocalData::m_iv_vecPunchAngle" ),
		m_iv_vecPunchAngleVel( "CPlayerLocalData::m_iv_vecPunchAngleVel" )
	{
		m_iv_vecPunchAngle.Setup( &m_vecPunchAngle.m_Value, LATCH_SIMULATION_VAR );
		m_iv_vecPunchAngleVel.Setup( &m_vecPunchAngleVel.m_Value, LATCH_SIMULATION_VAR );
		m_flFOVRate = 0;
	}

	unsigned char			m_chAreaBits[MAX_AREA_STATE_BYTES];				// Area visibility flags.
	unsigned char			m_chAreaPortalBits[MAX_AREA_PORTAL_STATE_BYTES];// Area portal visibility flags.

	int						m_iHideHUD;			// bitfields containing sections of the HUD to hide
	
	float					m_flFOVRate;		// rate at which the FOV changes
	

	bool					m_bDucked;
	bool					m_bDucking;
	bool					m_bInDuckJump;
	float					m_flDucktime;
	float					m_flDuckJumpTime;
	float					m_flJumpTime;
	int						m_nStepside;
	float					m_flFallVelocity;
	int						m_nOldButtons;
	// Base velocity that was passed in to server physics so 
	//  client can predict conveyors correctly.  Server zeroes it, so we need to store here, too.
	Vector					m_vecClientBaseVelocity;  
	CNetworkQAngle( m_vecPunchAngle );		// auto-decaying view angle adjustment
	CInterpolatedVar< QAngle >	m_iv_vecPunchAngle;

	CNetworkQAngle( m_vecPunchAngleVel );		// velocity of auto-decaying view angle adjustment
	CInterpolatedVar< QAngle >	m_iv_vecPunchAngleVel;
	bool					m_bDrawViewmodel;
	bool					m_bWearingSuit;
	bool					m_bPoisoned;
	float					m_flStepSize;
	bool					m_bAllowAutoMovement;

	// 3d skybox
	sky3dparams_t			m_skybox3d;
	// fog params
	fogplayerparams_t		m_PlayerFog;
	// audio environment
	audioparams_t			m_audio;

	bool					m_bSlowMovement;

};

#endif // C_PLAYERLOCALDATA_H
