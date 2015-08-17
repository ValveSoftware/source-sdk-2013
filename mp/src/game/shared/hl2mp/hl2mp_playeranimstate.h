//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef HL2MP_PLAYERANIMSTATE_H
#define HL2MP_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_HL2MP_Player;
#define CHL2MP_Player C_HL2MP_Player
#else
class CHL2MP_Player;
#endif

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CHL2MPPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CHL2MPPlayerAnimState, CMultiPlayerAnimState );

	CHL2MPPlayerAnimState();
	CHL2MPPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CHL2MPPlayerAnimState();

	void InitHL2MPAnimState( CHL2MP_Player *pPlayer );
	CHL2MP_Player *GetHL2MPPlayer( void )							{ return m_pHL2MPPlayer; }

	virtual void ClearAnimationState();
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );

	void	DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	bool	HandleMoving( Activity &idealActivity );
	bool	HandleJumping( Activity &idealActivity );
	bool	HandleDucking( Activity &idealActivity );
	bool	HandleSwimming( Activity &idealActivity );

	virtual float GetCurrentMaxGroundSpeed();

private:

	bool						SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void				EstimateYaw( void );
	virtual void				ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void				ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void				ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	
	CHL2MP_Player   *m_pHL2MPPlayer;
	bool		m_bInAirWalk;
	float		m_flHoldDeployedPoseUntilTime;
};

CHL2MPPlayerAnimState *CreateHL2MPPlayerAnimState( CHL2MP_Player *pPlayer );



#endif // HL2MP_PLAYERANIMSTATE_H
