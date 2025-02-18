//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#include "achievementmgr.h"
#include "baseachievement.h"

#ifdef GAME_DLL

#include "basegrenade_shared.h"

CAchievementMgr g_AchievementMgrEpisodic;	// global achievement mgr for episodic

class CAchievementEpXGetZombineGrenade : public CBaseAchievement
{
protected:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		if ( IsPC() )
		{
			// only in Ep2 for PC. (Shared across EPX for X360.)
			SetGameDirFilter( "ep2" );
		}
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "physgun_pickup" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "physgun_pickup" ) )
		{
			// was the object picked up a frag grenade?
			CBaseEntity *pEntityPickedUp = UTIL_EntityByIndex( event->GetInt( "entindex" ) );
			if ( pEntityPickedUp && pEntityPickedUp->ClassMatches( "npc_grenade_frag" ) )
			{
				// get the grenade object
				CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade *>( pEntityPickedUp );
				if ( pGrenade )
				{
					// was the original thrower a zombine?
					CBaseEntity *pOriginalThrower = pGrenade->GetOriginalThrower();
					if ( pOriginalThrower && pOriginalThrower->ClassMatches( "npc_zombine" ) )
					{
						IncrementCount();
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementEpXGetZombineGrenade, ACHIEVEMENT_EPX_GET_ZOMBINEGRENADE, "EPX_GET_ZOMBINEGRENADE", 5 );

class CAchievementEpXKillZombiesWithFlares : public CBaseAchievement
{
protected:
	void Init()
	{
		SetFlags( ACH_SAVE_WITH_GAME );
		SetGoal( 15 );

		if ( IsPC() )
		{
			// only in Ep1 for PC. (Shared across EPX for X360.)
			SetGameDirFilter( "episodic" );
		}
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "flare_ignite_npc" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "flare_ignite_npc" ) )
		{
			CBaseEntity *pEntityIgnited = UTIL_EntityByIndex( event->GetInt( "entindex" ) );
			// was it a zombie that got set on fire?
			if ( pEntityIgnited && 
				( ( pEntityIgnited->ClassMatches( "npc_zombie" ) ) || ( pEntityIgnited->ClassMatches( "npc_zombine" ) ) ||
				( pEntityIgnited->ClassMatches( "npc_fastzombie" ) ) ||  ( pEntityIgnited->ClassMatches( "npc_poisonzombie" ) ) ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementEpXKillZombiesWithFlares, ACHIEVEMENT_EPX_KILL_ZOMBIES_WITHFLARES, "EPX_KILL_ZOMBIES_WITHFLARES", 5 );

#endif // GAME_DLL
