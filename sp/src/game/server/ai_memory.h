//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		An NPC's memory of potential enemies 
//
//=============================================================================//

#include "mempool.h"
#include "utlmap.h"

#ifndef AI_MEMORY_H
#define AI_MEMORY_H
#pragma once

class CAI_Network;

DECLARE_POINTER_HANDLE(AIEnemiesIter_t);

const float AI_DEF_ENEMY_DISCARD_TIME = 60.0;

#define AI_UNKNOWN_ENEMY (((CBaseEntity *)NULL)+1) // use this to probe for unseen attackers
#define AI_INVALID_TIME (FLT_MAX * -1.0)

//-----------------------------------------------------------------------------
// AI_EnemyInfo_t
//
// Purpose: Stores relevant tactical information about an enemy
//
//-----------------------------------------------------------------------------
struct AI_EnemyInfo_t
{
	AI_EnemyInfo_t();
	
	EHANDLE			hEnemy;				// Pointer to the enemy

	Vector			vLastKnownLocation;
	Vector			vLastSeenLocation;
	float			timeLastSeen;		// Last time enemy was seen
	float			timeFirstSeen;		// First time enemy was seen
	float			timeLastReacquired;	
	float			timeValidEnemy;		// First time can be selected (reaction delay)
	float			timeLastReceivedDamageFrom;
	float			timeAtFirstHand;	// Time at which the enemy was seen firsthand
	bool			bDangerMemory;		// Memory of danger position w/o Enemy pointer
	bool			bEludedMe;			// True if enemy not at last known location 
	bool			bUnforgettable;
	bool			bMobbedMe;			// True if enemy was part of a mob at some point

	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
// CAI_Enemies
//
// Purpose: Stores a set of AI_EnemyInfo_t's
//
//-----------------------------------------------------------------------------
class CAI_Enemies
{
public:
	CAI_Enemies(void);
	~CAI_Enemies();
	
	AI_EnemyInfo_t *GetFirst( AIEnemiesIter_t *pIter );
	AI_EnemyInfo_t *GetNext( AIEnemiesIter_t *pIter );
	AI_EnemyInfo_t *Find( CBaseEntity *pEntity, bool bTryDangerMemory = false );
	AI_EnemyInfo_t *GetDangerMemory();

	int				NumEnemies() const		{ return m_Map.Count(); }
	int				GetSerialNumber() const	{ return m_serial;		}

	void			RefreshMemories(void);
	bool			UpdateMemory( CAI_Network* pAINet, CBaseEntity *enemy, const Vector &vPosition, float reactionDelay, bool firstHand );
	void			OnTookDamageFrom( CBaseEntity *pEnemy );

	bool			HasMemory( CBaseEntity *enemy );
	void			ClearMemory( CBaseEntity *enemy );

	const Vector &	LastKnownPosition( CBaseEntity *pEnemy );
	const Vector &	LastSeenPosition( CBaseEntity *pEnemy );

	float			TimeLastReacquired( CBaseEntity *pEnemy );
	float			LastTimeSeen( CBaseEntity *pEnemy, bool bCheckDangerMemory = true );
	float			FirstTimeSeen( CBaseEntity *pEnemy);
	bool			HasFreeKnowledgeOf( CBaseEntity *pEnemy );

	float			LastTimeTookDamageFrom( CBaseEntity *pEnemy);

	float			TimeAtFirstHand( CBaseEntity *pEnemy );
	
	void			MarkAsEluded( CBaseEntity *enemy );						// Don't know where he is (whole squad)
	bool			HasEludedMe( CBaseEntity *pEnemy );

	void			SetTimeValidEnemy( CBaseEntity *pEnemy, float flTime );

	void			SetUnforgettable( CBaseEntity *pEnemy, bool bUnforgettable = true );
	void			SetMobbedMe( CBaseEntity *pEnemy, bool bMobbedMe = true );
	
	void			SetFreeKnowledgeDuration( float flDuration );
	void			SetEnemyDiscardTime( float flTime );
	float			GetEnemyDiscardTime( void ) const { return m_flEnemyDiscardTime; }

	DECLARE_SIMPLE_DATADESC();

	typedef CUtlMap<CBaseEntity *, AI_EnemyInfo_t*, unsigned char> CMemMap;

private:
	bool ShouldDiscardMemory( AI_EnemyInfo_t *pMemory );

	CMemMap m_Map;
	float	m_flFreeKnowledgeDuration;
	float	m_flEnemyDiscardTime;
	Vector	m_vecDefaultLKP;
	Vector	m_vecDefaultLSP;
	int		m_serial;
};

//-----------------------------------------------------------------------------

#endif // AI_MEMORY_H
