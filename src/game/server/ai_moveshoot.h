//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_MOVESHOOT_H
#define AI_MOVESHOOT_H

#include "ai_component.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// @TODO (toml 07-09-03): probably want to fold this into base NPC. evaluate when
// head above water

class CAI_MoveAndShootOverlay : public CAI_Component
{
	typedef CAI_Component BaseClass;

public:
	CAI_MoveAndShootOverlay();

	void StartShootWhileMove( );
	void NoShootWhileMove();
	void RunShootWhileMove();
	void EndShootWhileMove();
	void SuspendMoveAndShoot( float flDuration );
	bool IsSuspended() { return m_flSuspendUntilTime > gpGlobals->curtime; }

	void SetInitialDelay( float delay );

	bool IsMovingAndShooting( void ) const { return m_bMovingAndShooting; }

private:

	bool HasAvailableRangeAttack();
	bool CanAimAtEnemy();
	void UpdateMoveShootActivity( bool bMoveAimAtEnemy );

	bool	m_bMovingAndShooting;
	bool	m_bNoShootWhileMove;
	float	m_initialDelay;
	float	m_flSuspendUntilTime;

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

#endif // AI_MOVESHOOT_H
