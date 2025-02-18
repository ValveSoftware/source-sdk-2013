//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity that propagates general data needed by clients for non-player AI characters
//
// $NoKeywords: $
//=============================================================================//

#ifndef MONSTER_RESOURCE_H
#define MONSTER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class CMonsterResource : public CBaseEntity
{
	DECLARE_CLASS( CMonsterResource, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual	int	 ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }

	virtual void Update( void );
	virtual int  UpdateTransmitState( void );

	virtual void SetBossHealthPercentage( float percentFull );	// if this is nonnegative, a HUD meter will be shown
	virtual void HideBossHealthMeter( void );

	virtual void SetBossStunPercentage( float percentFull );
	virtual void HideBossStunMeter( void );

	virtual void StartSkillShotComboMeter( float comboMaxDuration );
	virtual void IncrementSkillShotComboMeter( void );
	virtual void HideSkillShotComboMeter( void );

	void SetBossState( int iState ) { m_iBossState = iState; }

protected:
	CNetworkVar( int, m_iBossHealthPercentageByte );	// 0-255
	CNetworkVar( int, m_iBossStunPercentageByte );		// 0-255

	CNetworkVar( int, m_iSkillShotCompleteCount );		// the number of consecutive skill shots that have been completed. 0 = don't show combo HUD
	CNetworkVar( float, m_fSkillShotComboEndTime );		// the time when the current skill shot combo window closes

	CNetworkVar( int, m_iBossState );					// boss state?
};

extern CMonsterResource *g_pMonsterResource;

#endif // MONSTER_RESOURCE_H
