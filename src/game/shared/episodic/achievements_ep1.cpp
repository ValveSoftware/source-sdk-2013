//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"
#include "achievementmgr.h"
#include "baseachievement.h"

#ifdef GAME_DLL


class CAchievementEp1KillAntlionsWithCar : public CBaseAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "prop_physics" );
		SetVictimFilter( "npc_antlion" );
		SetGameDirFilter( "episodic" );
		SetGoal( 15 );
	}
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// any model that passed previous filters and begins with "props_vehicles" is a physics car
		const char *pszName = GetModelName( pInflictor );
		const char szPrefix[] = "props_vehicles";
		if ( 0 == Q_strncmp( pszName, szPrefix, ARRAYSIZE( szPrefix ) - 1 ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementEp1KillAntlionsWithCar, ACHIEVEMENT_EP1_KILL_ANTLIONS_WITHCARS, "EP1_KILL_ANTLIONS_WITHCARS", 5 );

class CAchievementEp1KillEnemiesWithSniperAlyx : public CBaseAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorEntityNameFilter( "sniper_alyx" );
		SetGameDirFilter( "episodic" );
		SetGoal( 30 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementEp1KillEnemiesWithSniperAlyx, ACHIEVEMENT_EP1_KILL_ENEMIES_WITHSNIPERALYX, "EP1_KILL_ENEMIES_WITHSNIPERALYX", 10 );

class CAchievementEp1BeatCitizenEscortNoCitizenDeaths : public CFailableAchievement
{
protected:

	void Init() 
	{
		SetFlags( ACH_LISTEN_MAP_EVENTS | ACH_LISTEN_KILL_EVENTS | ACH_SAVE_WITH_GAME );
		SetGameDirFilter( "episodic" );
		SetGoal( 1 );
		SetVictimFilter( "npc_citizen" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		// if any citizens die while this achievement is active, achievement fails
		SetFailed();
	}

	// map event where achievement is activated
	virtual const char *GetActivationEventName() { return "EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS_START"; }
	// map event where achievement is evaluated for success
	virtual const char *GetEvaluationEventName() { return "EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS_END"; }
};
DECLARE_ACHIEVEMENT( CAchievementEp1BeatCitizenEscortNoCitizenDeaths, ACHIEVEMENT_EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS, "EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS", 15 );

extern int CalcPlayerAttacks( bool bBulletOnly );

class CAchievementEp1BeatGameOneBullet : public CFailableAchievement
{
	DECLARE_CLASS( CAchievementEp1BeatGameOneBullet, CFailableAchievement );
protected:

	void Init() 
	{
		SetFlags( ACH_LISTEN_MAP_EVENTS | ACH_SAVE_WITH_GAME );
		SetGameDirFilter( "episodic" );
		SetGoal( 1 );
	}

	virtual void OnEvaluationEvent()
	{
		// get # of attacks w/bullet weapons
		int iBulletAttackCount = CalcPlayerAttacks( true );
		// if more than 1 bullet fired, fail
		if ( iBulletAttackCount > 1 )
		{
			SetFailed();
		}
		BaseClass::OnEvaluationEvent();
	}

	// map event where achievement is activated
	virtual const char *GetActivationEventName() { return "EP1_START_GAME"; }
	// map event where achievement is evaluated for success
	virtual const char *GetEvaluationEventName() { return "EP1_BEAT_GAME"; }

	// additional status for debugging
	virtual void PrintAdditionalStatus()
	{
		if ( m_bActivated )
		{
			Msg( "Player bullet attacks: %d\n", CalcPlayerAttacks( true ) );
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementEp1BeatGameOneBullet, ACHIEVEMENT_EP1_BEAT_GAME_ONEBULLET, "EP1_BEAT_GAME_ONEBULLET", 40 );

// Ep1-specific macro that sets game dir filter.  We need this because Ep1/Ep2/... share a binary so we need runtime check against running game.
#define DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( achievementID, achievementName, iPointValue )					\
	DECLARE_MAP_EVENT_ACHIEVEMENT_( achievementID, achievementName, "episodic", iPointValue, false )

// achievements which are won by a map event firing once
DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_EP1_BEAT_MAINELEVATOR, "EP1_BEAT_MAINELEVATOR", 5 );
DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_EP1_BEAT_CITADELCORE, "EP1_BEAT_CITADELCORE", 5 );
DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_EP1_BEAT_CITADELCORE_NOSTALKERKILLS, "EP1_BEAT_CITADELCORE_NOSTALKERKILLS", 10 );
DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_EP1_BEAT_GARAGEELEVATORSTANDOFF, "EP1_BEAT_GARAGEELEVATORSTANDOFF", 10 );
DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_EP1_BEAT_HOSPITALATTICGUNSHIP, "EP1_BEAT_HOSPITALATTICGUNSHIP", 5 );
DECLARE_EP1_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_EP1_BEAT_GAME, "EP1_BEAT_GAME", 20 );

#endif // GAME_DLL
