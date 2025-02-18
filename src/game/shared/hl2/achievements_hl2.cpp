//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#if GAME_DLL

#include "achievementmgr.h"
#include "baseachievement.h"

CAchievementMgr g_AchievementMgrHL2;	// global achievement mgr for HL2

class CAchievementHL2KillBarnaclesWithOneBarrel : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "prop_physics" );
		SetVictimFilter( "npc_barnacle" );
		SetGoal( 1 );
		m_pLastInflictor = NULL;
		m_iBarnacleCount = 0;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		int iDamageBits = event->GetInt( "damagebits" );
		// only interested blast damage.  (Barrels or other explosive phys objects are all OK)
		if ( !( iDamageBits & DMG_BLAST ) )
			return;

		if ( m_pLastInflictor != NULL && pInflictor != m_pLastInflictor )
		{
			m_iBarnacleCount = 1;
		}
		else
		{
			m_iBarnacleCount++;
			if ( 5 == m_iBarnacleCount )
			{
				IncrementCount();
			}			
		}
		m_pLastInflictor = pInflictor;
	}

	CBaseEntity *m_pLastInflictor;
	int m_iBarnacleCount;
};
DECLARE_ACHIEVEMENT( CAchievementHL2KillBarnaclesWithOneBarrel, ACHIEVEMENT_HL2_KILL_BARNACLESWITHBARREL, "HL2_KILL_BARNACLESWITHBARREL", 5 );

extern int CalcPlayerAttacks( bool bBulletOnly );

class CAchievementHL2BeatRavenholmNoWeapons : public CFailableAchievement
{
	DECLARE_CLASS( CAchievementHL2BeatRavenholmNoWeapons, CFailableAchievement );

	void Init()
	{
		SetFlags( ACH_LISTEN_MAP_EVENTS | ACH_SAVE_WITH_GAME );
		SetGoal( 1 );
		m_iInitialAttackCount = 0;
	}

	// map event where achievement is activated
	virtual const char *GetActivationEventName() { return "HL2_BEAT_RAVENHOLM_NOWEAPONS_START"; }
	// map event where achievement is evaluated for success
	virtual const char *GetEvaluationEventName() { return "HL2_BEAT_RAVENHOLM_NOWEAPONS_END"; }

	virtual void PreRestoreSavedGame() 
	{
		m_iInitialAttackCount = 0;
		BaseClass::PreRestoreSavedGame();
	}

	virtual void OnActivationEvent()
	{
		// get current # of attacks by player w/all weapons (except grav gun) and store that
		m_iInitialAttackCount = CalcPlayerAttacks( false );
		BaseClass::OnActivationEvent();
	}

	virtual void OnEvaluationEvent()
	{
		// get current # of attacks by player w/all weapons (except grav gun) 
		int iCurAttackCount = CalcPlayerAttacks( false );
		// compare to # of attacks when we started
		if ( iCurAttackCount > m_iInitialAttackCount )
		{
			// if there have been any more weapon attacks, achievement fails
			SetFailed();
		}
		BaseClass::OnEvaluationEvent();
	}

	// additional status for debugging
	virtual void PrintAdditionalStatus()
	{
		if ( m_bActivated )
		{
			Msg( "Starting wpn attacks: %d  Current wpn attacks: %d\n", m_iInitialAttackCount, CalcPlayerAttacks( false ) );
		}
	}

	int m_iInitialAttackCount;
public:
	DECLARE_DATADESC()
};
DECLARE_ACHIEVEMENT( CAchievementHL2BeatRavenholmNoWeapons, ACHIEVEMENT_HL2_BEAT_RAVENHOLM_NOWEAPONS, "HL2_BEAT_RAVENHOLM_NOWEAPONS", 25 );

BEGIN_DATADESC( CAchievementHL2BeatRavenholmNoWeapons )
DEFINE_FIELD( m_iInitialAttackCount,					FIELD_INTEGER ),
END_DATADESC()


class CAchievementHL2KillGunships : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetVictimFilter( "npc_combinegunship" );
		SetGoal( 6 );	// note: goal is really six, although #define is "THREEGUNSHIPS"
	}
};
DECLARE_ACHIEVEMENT( CAchievementHL2KillGunships, ACHIEVEMENT_HL2_KILL_THREEGUNSHIPS, "HL2_KILL_THREEGUNSHIPS", 5 );

class CAchievementHL2KillEnemiesWithAntlions : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "npc_antlion" );
		SetGoal( 50 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		if ( pPlayer )
		{
			// Only count antlion kills once player owns bugbait. 
			if ( pPlayer->Weapon_OwnsThisType( "weapon_bugbait" ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementHL2KillEnemiesWithAntlions, ACHIEVEMENT_HL2_KILL_ENEMIES_WITHANTLIONS, "HL2_KILL_ENEMIES_WITHANTLIONS", 10 );

class CAchievementHL2KillEnemyWithToilet : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME );
		SetInflictorFilter( "prop_physics" );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		const char *pszName = GetModelName( pInflictor );

		// skip past any directories and get just the file name
		pszName = V_UnqualifiedFileName( pszName );
		// if model name matches one of the toilets, this counts
		if ( ( 0 == Q_stricmp( pszName, "FurnitureToilet001a.mdl" ) ) || ( 0 == Q_stricmp( pszName, "prison_toilet01.mdl" ) ) )
		{
			IncrementCount();
		}		
	}
};
DECLARE_ACHIEVEMENT( CAchievementHL2KillEnemyWithToilet, ACHIEVEMENT_HL2_KILL_ENEMY_WITHTOILET, "HL2_KILL_ENEMY_WITHTOILET", 5 );

class CAchievementHL2DisintegrateSoldiersInField : public CBaseAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_WITH_GAME );
		SetGoal( 15 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "ragdoll_dissolved" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "ragdoll_dissolved" ) )
		{
			CBaseEntity *pRagdoll = UTIL_EntityByIndex( event->GetInt( "entindex", 0 ) );
			if ( pRagdoll )
			{
				const char *pszName = GetModelName( pRagdoll );

				// skip past any directories and get just the file name
				pszName = V_UnqualifiedFileName( pszName );
	
				if ( ( 0 == Q_stricmp( pszName, "combine_soldier.mdl" ) ) ||
					( 0 == Q_stricmp( pszName, "combine_super_soldier.mdl" ) ) )
				{
					IncrementCount();
				}							
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementHL2DisintegrateSoldiersInField, ACHIEVEMENT_HL2_DISINTEGRATE_SOLDIERSINFIELD, "HL2_DISINTEGRATE_SOLDIERSINFIELD", 10 );

class CAchievementHL2FindAllLambdas : public CBaseAchievement
{
	virtual void Init()
	{
		static const char *szComponents[] =
		{
			"HL2_LAMDACACHE_KLEINERSLAB", "HL2_LAMDACACHE_CANALSSTATION", "HL2_LAMDACACHE_VENTCRAWL", "HL2_LAMDACACHE_CANALSTUNNEL",
			"HL2_LAMDACACHE_SEWERGRATE", "HL2_LAMDACACHE_STEAMPIPE", "HL2_LAMDACACHE_CURVEDROOM", "HL2_LAMDACACHE_SHANTYTOWN",
			"HL2_LAMDACACHE_TUNNELLADDER", "HL2_LAMDACACHE_REDBARN", "HL2_LAMDACACHE_ZOMBIEAMBUSH", "HL2_LAMDACACHE_BELOWAPCS",
			"HL2_LAMDACACHE_COUNTERWEIGHT", "HL2_LAMDACACHE_RAILWAYBRIDGE", "HL2_LAMDACACHE_TUNNELPLATFORMS", "HL2_LAMDACACHE_BANKEDCANAL",
			"HL2_LAMDACACHE_CANALWALL", "HL2_LAMDACACHE_CHANNELSPLIT", "HL2_LAMDACACHE_BMEDOCK", "HL2_LAMDACACHE_GENERATORS",
			"HL2_LAMDACACHE_CARCRUSHERARENA", "HL2_LAMDACACHE_RAVENHOLMATTIC", "HL2_LAMDACACHE_MINETUNNELEXIT", 
			"HL2_LAMDACACHE_COASTSHACK", "HL2_LAMDACACHE_POISONSHACK", "HL2_LAMDACACHE_GUNSHIPVAN", "HL2_LAMDACACHE_SUICIDECITIZEN",
			"HL2_LAMDACACHE_RAILROADSHACK", "HL2_LAMDACACHE_COASTABOVEBATTERY", "HL2_LAMDACACHE_SANDSHACK", "HL2_LAMDACACHE_GMANCACHE",
			"HL2_LAMDACACHE_CELLCACHE", "HL2_LAMDACACHE_POISONLAUNDRY", "HL2_LAMDACACHE_SODAMACHINE",
			"HL2_LAMDACACHE_STREETWARDOGWALL", "HL2_LAMDACACHE_STREETWARSHACK", "HL2_LAMDACACHE_STREETWARFENCE", "HL2_LAMDACACHE_FREEWAYTUNNEL", "HL2_LAMDACACHE_DRAWBRIDGE",
			"HL2_LAMDACACHE_PLAZAFENCE", "HL2_LAMDACACHE_SEWERSCATWALKS", "HL2_LAMDACACHE_POISONZOMBIEALCOVE", "HL2_LAMDACACHE_PIPEHOPTUNNEL",
			"HL2_LAMDACACHE_ENDOFC1712B", "HL2_LAMDACACHE_EXITCATWALK"
		};		
		SetFlags( ACH_HAS_COMPONENTS | ACH_LISTEN_COMPONENT_EVENTS | ACH_SAVE_GLOBAL );
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetComponentPrefix( "HL2_LAMDACACHE" );
		SetGoal( m_iNumComponents );
	}
};
DECLARE_ACHIEVEMENT( CAchievementHL2FindAllLambdas, ACHIEVEMENT_HL2_FIND_ALLLAMBDAS, "HL2_FIND_ALLLAMBDAS", 15 );

// achievements which are won by a map event firing once
DECLARE_MAP_EVENT_ACHIEVEMENT_HIDDEN( ACHIEVEMENT_HL2_HIT_CANCOP_WITHCAN, "HL2_HIT_CANCOP_WITHCAN", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT_HIDDEN( ACHIEVEMENT_HL2_PUT_CANINTRASH, "HL2_PUT_CANINTRASH", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_ESCAPE_APARTMENTRAID, "HL2_ESCAPE_APARTMENTRAID", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BREAK_MINITELEPORTER, "HL2_BREAK_MINITELEPORTER", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_GET_CROWBAR, "HL2_GET_CROWBAR", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_GET_AIRBOAT, "HL2_GET_AIRBOAT", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_GET_AIRBOATGUN, "HL2_GET_AIRBOATGUN", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_FIND_VORTIGAUNTCAVE, "HL2_FIND_VORTIGAUNTCAVE", 13 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_KILL_CHOPPER, "HL2_KILL_CHOPPER", 10 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_FIND_HEVFACEPLATE, "HL2_FIND_HEVFACEPLATE", 10 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_GET_GRAVITYGUN, "HL2_GET_GRAVITYGUN", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_MAKEABASKET, "HL2_MAKEABASKET", 2 );
DECLARE_MAP_EVENT_ACHIEVEMENT_HIDDEN( ACHIEVEMENT_HL2_BEAT_CEMETERY, "HL2_BEAT_CEMETERY", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_KILL_ENEMIES_WITHCRANE, "HL2_KILL_ENEMIES_WITHCRANE", 5 );	
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_PIN_SOLDIER_TOBILLBOARD, "HL2_PIN_SOLDIER_TOBILLBOARD", 5 );		
DECLARE_MAP_EVENT_ACHIEVEMENT_HIDDEN( ACHIEVEMENT_HL2_KILL_ODESSAGUNSHIP, "HL2_KILL_ODESSAGUNSHIP", 5 );				
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BEAT_DONTTOUCHSAND, "HL2_BEAT_DONTTOUCHSAND", 20 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BEAT_TURRETSTANDOFF2, "HL2_BEAT_TURRETSTANDOFF2", 10 );			
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_FOLLOW_FREEMAN, "HL2_FOLLOWFREEMAN", 10 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BEAT_TOXICTUNNEL, "HL2_BEAT_TOXICTUNNEL", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BEAT_PLAZASTANDOFF, "HL2_BEAT_PLAZASTANDOFF", 10 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_KILL_ALLC1709SNIPERS, "HL2_KILL_ALLC1709SNIPERS", 5 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BEAT_SUPRESSIONDEVICE, "HL2_BEAT_SUPRESSIONDEVICE", 10 );
DECLARE_MAP_EVENT_ACHIEVEMENT( ACHIEVEMENT_HL2_BEAT_C1713STRIDERSTANDOFF, "HL2_BEAT_C1713STRIDERSTANDOFF", 10 );
DECLARE_MAP_EVENT_ACHIEVEMENT_HIDDEN( ACHIEVEMENT_HL2_BEAT_GAME, "HL2_BEAT_GAME", 25 );

#endif // GAME_DLL
