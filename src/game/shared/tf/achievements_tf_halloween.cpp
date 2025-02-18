//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"
#include "econ_wearable.h"
#include "achievements_tf.h"

// NVNT include for tf2 damage
#include "haptics/haptic_utils.h"

//-----------------------------------------------------------------------------
// Halloween Achievements
//-----------------------------------------------------------------------------

class CAchievementTFHalloweenCollectPumpkins : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		SetStoreProgressInSteam( true );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "halloween_pumpkin_grab" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
			return;

		if ( Q_strcmp( event->GetName(), "halloween_pumpkin_grab" ) == 0 )
		{
			int iPlayer = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( iPlayer );

			if ( pPlayer && pPlayer == C_TFPlayer::GetLocalTFPlayer() )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenCollectPumpkins, ACHIEVEMENT_TF_HALLOWEEN_COLLECT_PUMPKINS, "TF_HALLOWEEN_COLLECT_PUMPKINS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDominateForHat : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		bool bDomination = event->GetInt( "death_flags" ) & TF_DEATH_DOMINATION;

		if ( pTFVictim && pAttacker == C_TFPlayer::GetLocalTFPlayer() && bDomination == true )
		{
			// Are they wearing the HAT?
			for ( int i=0; i<pTFVictim->GetNumWearables(); ++i )
			{
				C_EconWearable *pWearable = pTFVictim->GetWearable( i );
				if ( pWearable && pWearable->GetAttributeContainer() )
				{
					CEconItemView *pItem = pWearable->GetAttributeContainer()->GetItem();
					if ( pItem && pItem->IsValid() )
					{
						if ( ( pItem->GetItemDefIndex() == 116 ) ||	// Ghastly Gibus
							 ( pItem->GetItemDefIndex() == 279 ) || 	// Ghastly Gibus 2010 
							 ( pItem->GetItemDefIndex() == 584 ) ||	// Ghastly Gibus 2011
							 ( pItem->GetItemDefIndex() == 940 ) )		// Ghostly Gibus
						{
							IncrementCount();
						}
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDominateForHat, ACHIEVEMENT_TF_HALLOWEEN_DOMINATE_FOR_HAT, "TF_HALLOWEEN_DOMINATE_FOR_HAT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenKillScaredPlayer : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( !TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
			return;

		if ( pVictim == pLocalPlayer )
			return;

		int iStunFlags = event->GetInt( "stun_flags" );
		bool bStunByTrigger = iStunFlags & TF_STUN_BY_TRIGGER;
		if ( bStunByTrigger )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenKillScaredPlayer, ACHIEVEMENT_TF_HALLOWEEN_KILL_SCARED_PLAYER, "TF_HALLOWEEN_KILL_SCARED_PLAYER", 1 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenPumpkinKill : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		SetStoreProgressInSteam( true );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( pLocalPlayer != pAttacker )
			return;

		if ( !TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
			return;

		if ( pVictim == pLocalPlayer )
			return;

		int customdmg = event->GetInt( "customkill" );
		if ( customdmg == TF_DMG_CUSTOM_PUMPKIN_BOMB )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenPumpkinKill, ACHIEVEMENT_TF_HALLOWEEN_PUMPKIN_KILL, "TF_HALLOWEEN_PUMPKIN_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDisguisedSpyKill : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
		if ( !pTFAttacker )
			return;

		CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( !pLocalPlayer )
			return;

		if ( !TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
			return;

		if ( pVictim == pLocalPlayer )
			return;

		if ( pTFVictim->m_Shared.GetDisguiseClass() == pTFAttacker->GetPlayerClass()->GetClassIndex() )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDisguisedSpyKill, ACHIEVEMENT_TF_HALLOWEEN_DISGUISED_SPY_KILL, "TF_HALLOWEEN_DISGUISED_SPY_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenBossKill : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenBossKill, ACHIEVEMENT_TF_HALLOWEEN_BOSS_KILL, "TF_HALLOWEEN_BOSS_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenBossKillMelee : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenBossKillMelee, ACHIEVEMENT_TF_HALLOWEEN_BOSS_KILL_MELEE, "TF_HALLOWEEN_BOSS_KILL_MELEE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenCollectGoodyBag : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// client awards this achievement in CGCHalloween_GrantedItemResponse, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenCollectGoodyBag, ACHIEVEMENT_TF_HALLOWEEN_COLLECT_GOODY_BAG, "TF_HALLOWEEN_COLLECT_GOODY_BAG", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenCraftSaxtonMask : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// client awards this achievement in CTFPlayerInventory::SOCreated(), no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenCraftSaxtonMask, ACHIEVEMENT_TF_HALLOWEEN_CRAFT_SAXTON_MASK, "TF_HALLOWEEN_CRAFT_SAXTON_MASK", 5 );
  
//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenEyeBossKill : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenEyeBossKill, ACHIEVEMENT_TF_HALLOWEEN_EYEBOSS_KILL, "TF_HALLOWEEN_EYEBOSS_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenLootIsland : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenLootIsland, ACHIEVEMENT_TF_HALLOWEEN_LOOT_ISLAND, "TF_HALLOWEEN_LOOT_ISLAND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenMerasmusKill : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenMerasmusKill, ACHIEVEMENT_TF_HALLOWEEN_MERASMUS_KILL, "TF_HALLOWEEN_MERASMUS_KILL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenMerasmusCollectLoot : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenMerasmusCollectLoot, ACHIEVEMENT_TF_HALLOWEEN_MERASMUS_COLLECT_LOOT, "TF_HALLOWEEN_MERASMUS_COLLECT_LOOT", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerRareSpell : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerRareSpell, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_RARE_SPELL, "TF_HALLOWEEN_HELLTOWER_RARE_SPELL", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerWinRounds : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 142 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "plr_hightower_event" );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			// Were we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerWinRounds, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_WIN_ROUNDS, "TF_HALLOWEEN_HELLTOWER_WIN_ROUNDS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerEnvironmentalKills : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 17 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerEnvironmentalKills, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_ENVIRONMENTAL_KILLS, "TF_HALLOWEEN_HELLTOWER_ENVIRONMENTAL_KILLS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerSkeletonGrind : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 99 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerSkeletonGrind, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKELETON_GRIND, "TF_HALLOWEEN_HELLTOWER_SKELETON_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerKillGrind : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 25 );
		SetStoreProgressInSteam( true );
		SetMapNameFilter( "plr_hightower_event" );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event ) 
	{
		if ( TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
		{
			CTFPlayer *pTFVictim = ToTFPlayer( pVictim );
			if ( !pTFVictim )
				return;

			CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
			if ( !pLocalPlayer )
				return;

			if ( pLocalPlayer != pAttacker )
				return;

			if ( pLocalPlayer == pVictim )
				return;

			switch( event->GetInt( "customkill" ) )
			{
			case TF_DMG_CUSTOM_SPELL_TELEPORT:
			case TF_DMG_CUSTOM_SPELL_SKELETON:
			case TF_DMG_CUSTOM_SPELL_MIRV:
			case TF_DMG_CUSTOM_SPELL_METEOR:
			case TF_DMG_CUSTOM_SPELL_LIGHTNING:
			case TF_DMG_CUSTOM_SPELL_FIREBALL:
			case TF_DMG_CUSTOM_SPELL_MONOCULUS:
			case TF_DMG_CUSTOM_SPELL_BLASTJUMP:
			case TF_DMG_CUSTOM_SPELL_BATS:
			case TF_DMG_CUSTOM_SPELL_TINY:
				IncrementCount();
				break;
			default:
				break;
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerKillGrind, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_KILL_GRIND, "TF_HALLOWEEN_HELLTOWER_KILL_GRIND", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerKillBrothers : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerKillBrothers, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_KILL_BROTHERS, "TF_HALLOWEEN_HELLTOWER_KILL_BROTHERS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerMilestone : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFHalloweenHelltowerMilestone, CAchievement_AchievedCount );
	void Init() 
	{
		BaseClass::Init();
		SetAchievementsRequired( 4, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_RARE_SPELL, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_KILL_BROTHERS );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerMilestone, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_MILESTONE, "TF_HALLOWEEN_HELLTOWER_MILESTONE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenHelltowerSkullIslandReward : public CBaseTFAchievementSimple
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenHelltowerSkullIslandReward, ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKULL_ISLAND_REWARD, "TF_HALLOWEEN_HELLTOWER_SKULL_ISLAND_REWARD", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayKillKarts : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 30 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayKillKarts, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_KILL_KARTS, "TF_HALLOWEEN_DOOMSDAY_KILL_KARTS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayCollectDucks : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 250 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayCollectDucks, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_COLLECT_DUCKS, "TF_HALLOWEEN_DOOMSDAY_COLLECT_DUCKS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayScoreGoals : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 3 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayScoreGoals, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_SCORE_GOALS, "TF_HALLOWEEN_DOOMSDAY_SCORE_GOALS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayRespawnTeammates : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 30 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayRespawnTeammates, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_RESPAWN_TEAMMATES, "TF_HALLOWEEN_DOOMSDAY_RESPAWN_TEAMMATES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayTinySmasher : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 15 );
		SetStoreProgressInSteam( true );
	}

	// server awards this achievement, no other code within achievement necessary
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayTinySmasher, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_TINY_SMASHER, "TF_HALLOWEEN_DOOMSDAY_TINY_SMASHER", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayWinMinigames : public CBaseTFAchievementSimple
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS );
		SetGoal( 3 );
		SetMapNameFilter( "sd_doomsday_event" );
	}


	virtual void ListenForEvents()
	{
		ListenForGameEvent( "minigame_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "minigame_win" ) )
		{
			// Are we on the winning team?
			int iTeam = event->GetInt( "team" );
			if ( ( iTeam >= FIRST_GAME_TEAM ) && ( iTeam == GetLocalPlayerTeam() ) )
			{
				EnsureComponentBitSetAndEvaluate( event->GetInt( "type" ) );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayWinMinigames, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_WIN_MINIGAMES, "TF_HALLOWEEN_DOOMSDAY_WIN_MINIGAMES", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFHalloweenDoomsdayMilestone : public CAchievement_AchievedCount
{
public:
	DECLARE_CLASS( CAchievementTFHalloweenDoomsdayMilestone, CAchievement_AchievedCount );
	void Init()
	{
		BaseClass::Init();
		SetAchievementsRequired( 4, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_KILL_KARTS, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_WIN_MINIGAMES );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFHalloweenDoomsdayMilestone, ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_MILESTONE, "TF_HALLOWEEN_DOOMSDAY_MILESTONE", 5 );

#endif // CLIENT_DLL
